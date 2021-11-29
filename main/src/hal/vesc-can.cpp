#include <hal/vesc-can.h>

#ifdef HAS_VESC_CAN
FreeRTOS::Semaphore VescCan::canReceive = FreeRTOS::Semaphore("canReceive");
FreeRTOS::Semaphore VescCan::canTransmit = FreeRTOS::Semaphore("canTransmit");

VescCan::VescCan()
{
  can_general_config_t g_config = CAN_GENERAL_CONFIG_DEFAULT(CAN_TX, CAN_RX, CAN_MODE_NORMAL);
  can_timing_config_t t_config = CAN_TIMING_CONFIG_500KBITS();
  can_filter_config_t f_config = CAN_FILTER_CONFIG_ACCEPT_ALL();

  auto ret = can_driver_install(&g_config, &t_config, &f_config);
  ESP_ERROR_CHECK(ret);

  ret = can_start();
  ESP_ERROR_CHECK(ret);

  transmitQueue = xQueueCreate(1000, sizeof(std::string *));
  receiveQueue = xQueueCreate(1000, sizeof(std::string *));

  ESP_LOGI(CAN_TAG, "Setup CAN driver");
}

void VescCan::startup()
{
  delay(100);
  for (auto listener : lifecycleListeners)
    listener->onPowerUp();

  // start up CAN listener
  xTaskCreatePinnedToCore(canTask, "can-processor", 4096, this, 2, &canHandle, 0);
}

void VescCan::canTask(void *parameters)
{
  VescCan *can = (VescCan *)parameters;

  for (;;)
  {
    VehicleState old = can->_vehicleState;

    can->process();

    if (can->initialized)
    {
      if (old != can->_vehicleState)
      {
        ESP_LOGD(MOTION_TAG, "vehicle state changed in motion");

        for (auto listener : can->motionListeners)
          if (listener->vehicleQueue != NULL)
            xQueueSendToFront(listener->vehicleQueue, (void *)&can->_vehicleState, 0);

        old = can->_vehicleState;
      }

      PowerStatus newPowerStatus = can->calculatePowerStatus(true, false, false, percentageFromReading(can->status.realtime.inputVoltage, Config::ampConfig.battery.voltageBreakpoints));
      ESP_LOGV(CAN_TAG, "Power Status: %d (%.2fV)", newPowerStatus.percentage, can->status.realtime.inputVoltage);
      if (can->powerStatus != newPowerStatus)
      {
        can->powerStatus = newPowerStatus;
        can->notifyPowerListeners();
      }
    }
  }
}

void VescCan::process()
{
  can_message_t message;
  frameCount = 0;

  if (uxQueueMessagesWaiting(receiveQueue))
  {
    std::string *receiveBuffer = NULL;
    xQueueReceive(receiveQueue, &receiveBuffer, 0);
    proxyIn(*receiveBuffer);
    delete receiveBuffer;
    canReceive.give();
  }

  if (uxQueueMessagesWaiting(configUpdatedQueue))
  {
    bool valid;
    if (xQueueReceive(configUpdatedQueue, &valid, 0) && valid)
      onConfigUpdated();
  }

  if (initialized)
  {
    if (millis() - status.realtime.timestamp > interval)
      requestRealtimeData();

    if (millis() - status.balance.timestamp > interval / 2)
      requestBalanceData();
  }
  else if (initRetryCounter > 0 && millis() - lastRetry > 500)
  {
    ESP_LOGI(CAN_TAG, "Request can info for initialization");
    requestFirmwareVersion();
    initRetryCounter--;
    lastRetry = millis();
    if (initRetryCounter == 0)
    {
      ESP_LOGE(CAN_TAG, "CAN initialization failed");
      return;
    }
  }

  while (can_receive(&message, portTICK_PERIOD_MS) == ESP_OK)
  {
    frameCount++;

    //Process received message
    if (!message.extd)
    {
      ESP_LOGW(CAN_TAG, "Ignoring standard message");
      return;
    }

    if (frameCount > 1000)
    {
      ESP_LOGW(CAN_TAG, "Too many frames in one loop");
      return;
    }

    processFrame(message);
  }

  delay(10);
}

void VescCan::requestFirmwareVersion()
{
  can_message_t message;

  message.extd = 1;
  message.identifier = (uint32_t(0x8000) << 16) + (uint16_t(CAN_PACKET_PROCESS_SHORT_BUFFER) << 8) + VESC_ID;
  message.data_length_code = 0x03;

  message.data[0] = AMP_ID;
  message.data[1] = 0x00;
  message.data[2] = 0x00; // COMM_FW_VERSION

  sendFrame(&message);
}

void VescCan::requestRealtimeData()
{
  ESP_LOGV(CAN_TAG, "Request realtime data");
  can_message_t message;

  message.extd = 1;
  message.identifier = (uint32_t(0x8000) << 16) + (uint16_t(CAN_PACKET_PROCESS_SHORT_BUFFER) << 8) + VESC_ID;
  message.data_length_code = 0x07;
  message.data[0] = AMP_ID;
  message.data[1] = 0x00;
  message.data[2] = 0x32; // COMM_GET_VALUES_SELECTIVE
  // mask
  message.data[3] = 0x00;       // Byte1 of mask (Bits 24-31)
  message.data[4] = 0x00;       // Byte2 of mask (Bits 16-23)
  message.data[5] = 0B10000111; // Byte3 of mask (Bits 8-15)
  message.data[6] = 0B11000011; // Byte4 of mask (Bits 0-7)
  sendFrame(&message);
}

void VescCan::requestBalanceData()
{
  ESP_LOGV(CAN_TAG, "Request balance data");
  can_message_t message;

  message.extd = 1;
  message.identifier = (uint32_t(0x8000) << 16) + (uint16_t(CAN_PACKET_PROCESS_SHORT_BUFFER) << 8) + VESC_ID;
  message.data_length_code = 0x03;

  message.data[0] = AMP_ID;
  message.data[1] = 0x00;
  message.data[2] = 0x4F;

  sendFrame(&message);
}

void VescCan::ping()
{
  can_message_t message;

  message.extd = 1;
  message.identifier = (uint32_t(0x8000) << 16) + (uint16_t(CAN_PACKET_PING) << 8) + VESC_ID;
  message.data_length_code = 0x01;

  message.data[0] = AMP_ID;

  sendFrame(&message);
}

void VescCan::processFrame(can_message_t message)
{
  bool longBuffer, isProxyRequest;
  uint8_t command;
  int i;

  switch (message.identifier)
  {
  case RECV_STATUS_1:
    ESP_LOGV(CAN_TAG, "Match recv status 1: %d", message.identifier);
    status.realtime.erpm = readInt32(message, 0);
    status.realtime.current = readInt32(message, 4) / 10.0;
    status.realtime.dutyCycle = readInt32(message, 6);
    ESP_LOGV(CAN_TAG, "erpm: %.2f, current: %.2f, dutyCycle: %.2f", status.realtime.erpm, status.realtime.current, status.realtime.dutyCycle);

    if (Config::ampConfig.motion.autoDirection)
      detectDirection();
    break;
  case RECV_STATUS_2:
    ESP_LOGV(CAN_TAG, "Match recv status 2: %d", message.identifier);
    status.realtime.ampHours = readInt32(message, 0) / 10000.0;
    status.realtime.ampHoursCharged = readInt32(message, 4) / 10000.0;
    ESP_LOGV(CAN_TAG, "amp hours: %.2f, amp hours charged: %.2f", status.realtime.ampHours, status.realtime.ampHoursCharged);
    break;
  case RECV_STATUS_3:
    ESP_LOGV(CAN_TAG, "Match recv status 3: %d", message.identifier);
    status.realtime.wattHours = readInt32(message, 0) / 10000.0;
    status.realtime.wattHoursCharged = readInt32(message, 4) / 10000.0;
    ESP_LOGV(CAN_TAG, "watt hours: %.2f, watt hours charged: %.2f", status.realtime.wattHours, status.realtime.wattHoursCharged);
    break;
  case RECV_STATUS_4:
    ESP_LOGV(CAN_TAG, "Match recv status 4: %d", message.identifier);
    status.realtime.mosfetTemp = readInt16(message, 0) / 10.0;
    status.realtime.motorTemp = readInt16(message, 2) / 10.0;
    status.realtime.totalCurrentIn = readInt16(message, 4) / 10.0;
    status.realtime.pidPosition = readInt16(message, 6) / 50.0;
    ESP_LOGV(CAN_TAG, "mosfet temp: %.2f, motor temp: %.2f, total current in: %.2f, pid position: %.2f", status.realtime.mosfetTemp, status.realtime.motorTemp, status.realtime.totalCurrentIn, status.realtime.pidPosition);
    break;
  case RECV_STATUS_5:
    ESP_LOGV(CAN_TAG, "Match recv status 5: %d", message.identifier);
    status.realtime.tachometer = readInt32(message, 0);
    status.realtime.inputVoltage = readInt16(message, 4) / 10.0;
    ESP_LOGV(CAN_TAG, "tachometer: %.2f, input voltage: %.2f", status.realtime.tachometer, status.realtime.inputVoltage);
    break;
  case RECV_PROCESS_SHORT_BUFFER_PROXY:
    ESP_LOGI(CAN_TAG, "Match process short buffer proxy: %d", message.identifier);
    // process short buffer for proxy
    for (i = 1; i < message.data_length_code; i++)
      proxybuffer.push_back(message.data[i]);

    proxyOut(proxybuffer.data(), proxybuffer.size(), message.data[4], message.data[5]);
    proxybuffer.clear();
    break;
  case RECV_FILL_RX_BUFFER:
    ESP_LOGV(CAN_TAG, "Match fill rx buffer: %d", message.identifier);
    // fill receive buffer
    for (i = 1; i < message.data_length_code; i++)
      buffer.push_back(message.data[i]);

    break;
  case RECV_FILL_RX_BUFFER_PROXY:
  case RECV_FILL_RX_BUFFER_LONG_PROXY:
    ESP_LOGV(CAN_TAG, "Match recv fill rx buffer proxy (%d) or recv fill rx buffer long proxy (%d): %d", RECV_FILL_RX_BUFFER_PROXY, RECV_FILL_RX_BUFFER_LONG_PROXY, message.identifier);
    // fill receive buffer for proxy / long proxy
    longBuffer = RECV_FILL_RX_BUFFER_LONG_PROXY == message.identifier;
    for (i = (longBuffer ? 2 : 1); i < message.data_length_code; i++)
    {
      proxybuffer.push_back(message.data[i]);
    }
    break;
  case RECV_PROCESS_RX_BUFFER:
  case RECV_PROCESS_RX_BUFFER_PROXY:
    ESP_LOGV(CAN_TAG, "Match recv process rx buffer proxy (%d) or recv process rx buffer proxy (%d): %d", RECV_PROCESS_RX_BUFFER, RECV_PROCESS_RX_BUFFER_PROXY, message.identifier);
    // process receive buffer for self / proxy
    isProxyRequest = RECV_PROCESS_RX_BUFFER_PROXY == message.identifier;

    if ((!isProxyRequest && buffer.size() <= 0) || (isProxyRequest && proxybuffer.size() <= 0))
    {
      ESP_LOGW(CAN_TAG, "Empty buffer - no need to process");
      return;
    }

    command = (isProxyRequest ? proxybuffer : buffer).at(0);
    processBufferedData(command, isProxyRequest);
    if (isProxyRequest)
    {
      proxyOut(proxybuffer.data(), proxybuffer.size(), message.data[4], message.data[5]);
      proxybuffer.clear();
    }
    else
    {
      buffer.clear();
    }
    break;
  default:
    break;
    // ESP_LOGW(CAN_TAG, "Unknown can identifier %d, DLC: %d", message.identifier, message.data_length_code);
    // ESP_LOGW(CAN_TAG, "Unknown data: %d, %d, %d, %d, %d, %d, %d", message.data[0], message.data[1], message.data[2], message.data[3], message.data[4], message.data[5], message.data[6]);
  }
}

void VescCan::processBufferedData(uint8_t command, bool isProxyRequest)
{
  ESP_LOGV(CAN_TAG, "process buffered data");
  switch (command)
  {
  case 0x00:
    parseFirmwareData(isProxyRequest);
    break;
  case 0x4F:
    parseBalanceData(isProxyRequest);
    break;
  case 0x32:
    parseRealtimeData(isProxyRequest);
    break;
  default:
    ESP_LOGW(CAN_TAG, "unknown buffered command: %d", command);
    break;
  }
}

void VescCan::parseFirmwareData(bool isProxyRequest)
{
  // firmwareVersion
  int offset = 1;
  status.firmwareVersion.major = readInt8FromBuffer(0, isProxyRequest);
  status.firmwareVersion.minor = readInt8FromBuffer(1, isProxyRequest);
  status.firmwareVersion.name = readStringFromBuffer(2 + offset, 12, isProxyRequest);
  ESP_LOGV(CAN_TAG, "firmware: %d.%d - %s", status.firmwareVersion.major, status.firmwareVersion.minor, status.firmwareVersion.name.c_str());
  if (!initialized)
    initialized = true;
}

void VescCan::parseBalanceData(bool isProxyRequest)
{
  // balance
  int offset = 1;
  status.balance.pidOutput = readInt32FromBuffer(0 + offset, isProxyRequest) / 1000000.0;
  status.balance.pitch = readInt32FromBuffer(4 + offset, isProxyRequest) / 1000000.0;
  status.balance.roll = readInt32FromBuffer(8 + offset, isProxyRequest) / 1000000.0;
  status.balance.loopTime = readInt32FromBuffer(12 + offset, isProxyRequest);
  status.balance.motorCurrent = readInt32FromBuffer(16 + offset, isProxyRequest) / 1000000.0;
  status.balance.motorPosition = readInt32FromBuffer(20 + offset, isProxyRequest) / 1000000.0;
  status.balance.balanceState = (VescBalanceState)readInt16FromBuffer(24 + offset, isProxyRequest);
  status.balance.switchState = (VescSwitchState)readInt16FromBuffer(26 + offset, isProxyRequest);
  status.balance.adc1 = readInt32FromBuffer(28 + offset, isProxyRequest) / 1000000.0;
  status.balance.adc2 = readInt32FromBuffer(32 + offset, isProxyRequest) / 1000000.0;
  status.balance.timestamp = millis();
  ESP_LOGV(CAN_TAG, "roll: %.2f, pitch: %.2f, balance: %d, switch: %d, adc1: %.2f, adc2: %.2f", status.balance.roll, status.balance.pitch, status.balance.balanceState, status.balance.switchState, status.balance.adc1, status.balance.adc2);

  updateBalance();
}

void VescCan::parseRealtimeData(bool isProxyRequest)
{
  // realtime data
  int offset = 1;
  status.realtime.mosfetTemp = readInt16FromBuffer(4 + offset, isProxyRequest) / 10.0;
  status.realtime.motorTemp = readInt16FromBuffer(6 + offset, isProxyRequest) / 10.0;
  status.realtime.dutyCycle = readInt16FromBuffer(8 + offset, isProxyRequest) / 1000.0;
  status.realtime.erpm = readInt32FromBuffer(10 + offset, isProxyRequest);
  status.realtime.inputVoltage = readInt16FromBuffer(14 + offset, isProxyRequest) / 10.0;
  status.realtime.tachometer = readInt32FromBuffer(16 + offset, isProxyRequest);
  status.realtime.tachometerAbsolute = readInt32FromBuffer(20 + offset, isProxyRequest);
  status.realtime.fault = readInt8FromBuffer(24 + offset, isProxyRequest);
  status.realtime.timestamp = millis();
  ESP_LOGV(CAN_TAG, "mosfet temp: %.2f, motor temp: %.2f, duty cycle: %.2f, erpm: %.2f, input voltage: %.2f, tachomter: %.2f, tachometer (abs): %.2f, fault: %d", status.realtime.mosfetTemp, status.realtime.motorTemp, status.realtime.dutyCycle, status.realtime.erpm, status.realtime.inputVoltage, status.realtime.tachometer, status.realtime.tachometerAbsolute, status.realtime.fault);

  if (Config::ampConfig.motion.autoDirection)
    detectDirection();
}

void VescCan::sendFrame(can_message_t *message)
{
  if (!can_transmit(message, pdMS_TO_TICKS(1000)) == ESP_OK)
    ESP_LOGW(CAN_TAG, "Failed to queue message for transmission");
}

float VescCan::getDirectionFromAxis()
{
  float value = 0;
  switch (_directionTrigger)
  {
  case DirectionTrigger::Acceleration:
    // todo
    break;
  case DirectionTrigger::Attitude:
    switch (_directionAttitudeAxis)
    {
    case AttitudeAxis::Roll:
      value = status.balance.roll;
      break;
    case AttitudeAxis::Roll_Invert:
      value = -status.balance.roll;
      break;
    case AttitudeAxis::Pitch:
      value = status.balance.pitch;
      break;
    case AttitudeAxis::Pitch_Invert:
      value = -status.balance.pitch;
      break;
    default:
      //todo
      break;
    }
    break;
  }
  return value;
}

bool VescCan::updateBalance()
{
  ESP_LOGV(CAN_TAG, "Balance state: %d, %.2f / %.2f, %.2f / %.2f", status.balance.balanceState, status.balance.adc1, Config::ampConfig.balance.footpads.adc1, status.balance.adc2, Config::ampConfig.balance.footpads.adc2);
  BalanceState newBalanceState;
  switch (status.balance.balanceState)
  {
  case FAULT_SWITCH_HALF:
  case FAULT_SWITCH_FULL:
    newBalanceState.faultRight = status.balance.adc1 < Config::ampConfig.balance.footpads.adc1;
    newBalanceState.faultLeft = status.balance.adc2 < Config::ampConfig.balance.footpads.adc2;
    break;
  default:
    newBalanceState.faultRight = false;
    newBalanceState.faultLeft = false;
    break;
  }

  newBalanceState.operatingState = status.balance.balanceState;

  ESP_LOGV(CAN_TAG, "New balance state: %s, %s", newBalanceState.faultLeft ? "yes" : "no", newBalanceState.faultRight ? "yes" : "no");
  VehicleState newVehicleState = _vehicleState;
  newVehicleState.balance = newBalanceState;
  triggerVehicleState(newVehicleState, _autoMotion, _autoTurn, _autoOrientation);
  return true;
}

bool VescCan::detectMotion()
{
  // todo
  return false;
}

bool VescCan::detectDirection()
{
  Direction newDirection = _vehicleState.direction;
  // float value = getDirectionFromAxis();
  // uint8_t axisValue = DirectionTrigger::Acceleration ? _directionAccelerationAxis : _directionAttitudeAxis;
  float value = -status.realtime.erpm;
  float threshold = Config::ampConfig.motion.directionThreshold;
  ESP_LOGW(CAN_TAG, "ERPM: %.2f", value);

  switch (_vehicleState.direction)
  {
  case Direction::Backward:
    if (value >= threshold)
      newDirection = Direction::Forward;
    break;
  case Direction::Forward:
    if (value <= -threshold)
      newDirection = Direction::Backward;
    break;
  }

  if (_vehicleState.direction != newDirection)
  {
    ESP_LOGW(CAN_TAG, "Direction changed");
    triggerDirectionState(newDirection, _autoDirection);
    return true;
  }

  return false;
}

int16_t VescCan::readInt16(can_message_t message, int start)
{
  int16_t intVal = (((int16_t)message.data[start] << 8) +
                    ((int16_t)message.data[start + 1]));
  return intVal;
}

int32_t VescCan::readInt32(can_message_t message, int start)
{
  int32_t intVal = (((int32_t)message.data[start] << 24) +
                    ((int32_t)message.data[start + 1] << 16) +
                    ((int32_t)message.data[start + 2] << 8) +
                    ((int32_t)message.data[start + 3]));
  return intVal;
}

int8_t VescCan::readInt8FromBuffer(int start, bool isProxyRequest)
{
  return (isProxyRequest ? proxybuffer : buffer).at(start);
}

int16_t VescCan::readInt16FromBuffer(int start, bool isProxyRequest)
{
  int16_t intVal = (((int16_t)(isProxyRequest ? proxybuffer : buffer).at(start) << 8) +
                    ((int16_t)(isProxyRequest ? proxybuffer : buffer).at(start + 1)));
  return intVal;
}

int32_t VescCan::readInt32FromBuffer(int start, bool isProxyRequest)
{
  int32_t intVal = (((int32_t)(isProxyRequest ? proxybuffer : buffer).at(start) << 24) +
                    ((int32_t)(isProxyRequest ? proxybuffer : buffer).at(start + 1) << 16) +
                    ((int32_t)(isProxyRequest ? proxybuffer : buffer).at(start + 2) << 8) +
                    ((int32_t)(isProxyRequest ? proxybuffer : buffer).at(start + 3)));
  return intVal;
}

std::string VescCan::readStringFromBuffer(int start, int length, bool isProxyRequest)
{
  std::string name = "";
  for (int i = start; i < start + length; i++)
    name += (char)(isProxyRequest ? proxybuffer : buffer).at(i);
  return name;
}

void VescCan::proxyIn(std::string in)
{
  uint8_t packet_type = (uint8_t)in.at(0);

  if (!longPacket)
  {
    switch (packet_type)
    {
    case 2:
      length = (uint8_t)in.at(1);
      command = (uint8_t)in.at(2);
      break;
    case 3:
      length = ((uint8_t)in.at(1) << 8) + (uint8_t)in.at(2);
      command = (uint8_t)in.at(3);
      longPacket = true;
      break;
    default:
      return;
    }
  }

  if (longPacket)
  {
    longPackBuffer += in;
    // 7 bytes overhead
    if (length > (longPackBuffer.size() - 6))
    {
      ESP_LOGW(CAN_TAG, "Buffer not full - only filled %d/%d", length, longPackBuffer.size() - 6);
      return;
    }

    ESP_LOGV(CAN_TAG, "Buffer full, processing");

    unsigned int end_a = 0;
    int offset = 2;
    // skip first two bytes CRC here
    for (unsigned int byteNum = offset; byteNum < length; byteNum += 7)
    {
      if (byteNum > 255 + offset)
      {
        break;
      }

      end_a = byteNum + 7;

      can_message_t message;
      message.extd = 1;
      message.identifier = (uint32_t(0x8000) << 16) + (uint16_t(CAN_PACKET_FILL_RX_BUFFER) << 8) + VESC_ID;
      message.data_length_code = 8;
      message.data[0] = byteNum - offset; //startbyte counter of frame

      int sendLen = (longPackBuffer.length() >= byteNum + 7) ? 7 : longPackBuffer.length() - byteNum;
      for (int i = 1; i < sendLen + 1; i++)
      {
        message.data[i] = (uint8_t)longPackBuffer.at(byteNum + i);
      }
      sendFrame(&message);
    }

    for (unsigned int byteNum = end_a - 1; byteNum < length; byteNum += 6)
    {
      can_message_t message;
      message.extd = 1;
      message.identifier = (uint32_t(0x8000) << 16) + (uint16_t(CAN_PACKET_FILL_RX_BUFFER_LONG) << 8) + VESC_ID;
      message.data_length_code = 8;
      message.data[0] = (byteNum - 1) >> 8;
      message.data[1] = (byteNum - 1) & 0xFF;

      int sendLen = (longPackBuffer.length() >= byteNum + 6) ? 6 : longPackBuffer.length() - byteNum;
      for (int i = 2; i < sendLen + 2; i++)
      {
        message.data[i] = (uint8_t)longPackBuffer.at(byteNum + i);
      }

      sendFrame(&message);
    }

    can_message_t message;
    message.extd = 1;
    message.identifier = (uint32_t(0x8000) << 16) + (uint16_t(CAN_PACKET_PROCESS_RX_BUFFER) << 8) + VESC_ID;
    message.data_length_code = 6;
    message.data[0] = BLE_PROXY_ID;
    message.data[1] = 0; // IS THIS CORRECT?????
    message.data[2] = length >> 8;
    message.data[3] = length & 0xFF;
    message.data[4] = longPackBuffer.at(longPackBuffer.size() - 3);
    message.data[5] = longPackBuffer.at(longPackBuffer.size() - 2);
    sendFrame(&message);
  }
  else
  {
    can_message_t message;
    message.extd = 1;
    message.identifier = (uint32_t(0x8000) << 16) + (uint16_t(CAN_PACKET_PROCESS_SHORT_BUFFER) << 8) + VESC_ID;
    message.data_length_code = 0x02 + length;
    message.data[0] = BLE_PROXY_ID;
    message.data[1] = 0x00;
    message.data[2] = command;
    for (int i = 3; i < length + 2; i++)
    {
      message.data[i] = (uint8_t)in.at(i);
    }
    sendFrame(&message);
  }

  length = 0;
  command = 0;
  longPacket = false;
  longPackBuffer = "";
}

void VescCan::proxyOut(uint8_t *data, int size, uint8_t crc1, uint8_t crc2)
{
  if (size > BUFFER_SIZE)
  {
    ESP_LOGE(CAN_TAG, "Buffer size exceeded for proxying out - aborting");
    return;
  }

  std::stringstream stream;

  //Start bit, package size
  if (size <= 255)
  {
    stream << 0x02;
    stream << size;
  }
  else if (size <= 65535)
  {
    stream << 0x03;
    stream << (size >> 8);
    stream << (size & 0xFF);
  }
  else
  {
    stream << 0x04;
    stream << (size >> 16);
    stream << ((size >> 8) & 0x0F);
    stream << (size & 0xFF);
  }

  // data
  for (int i = 0; i < size; i++)
  {
    stream << data[i];
  }

  //crc 2 byte
  stream << crc1;
  stream << crc2;

  // Stop bit
  stream << 0x03;

  std::string *pStr = new std::string(stream.str());
  VescCan::canTransmit.wait();
  VescCan::canTransmit.take();
  xQueueSendToBack(transmitQueue, &pStr, 0);
}
#endif