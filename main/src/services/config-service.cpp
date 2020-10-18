#include <services/config-service.h>

ConfigService::ConfigService(Config *config, NimBLEServer *server) {
  _config = config;
  _server = server;

  setupService();
}

void ConfigService::setupService() {
  auto service = _server->createService(configServiceUUID);

  _configRxCharacteristic = service->createCharacteristic(
    NimBLEUUID::fromString(configRxCharacteristicUUID),
    NIMBLE_PROPERTY::WRITE | 
    NIMBLE_PROPERTY::WRITE_NR);
  
  _configRxCharacteristic->setCallbacks(this);

  _configTxCharacteristic = service->createCharacteristic(
    NimBLEUUID::fromString(configTxCharacteristicUUID),
    NIMBLE_PROPERTY::READ |
    NIMBLE_PROPERTY::NOTIFY);

  _configStatusCharacteristic = service->createCharacteristic(
    NimBLEUUID::fromString(configStatusCharacteristicUUID),
    NIMBLE_PROPERTY::WRITE |
    NIMBLE_PROPERTY::READ |
    NIMBLE_PROPERTY::NOTIFY
  );

  _configStatusCharacteristic->setCallbacks(this);

  service->start();
}

void ConfigService::onWrite(NimBLECharacteristic* characteristic) {
  std::string uuid = characteristic->getUUID().toString();
  std::string received = characteristic->getValue();

  if (uuid.compare(configStatusCharacteristicUUID) == 0) {
    const char* data = received.data();

    // start buffer
    if (data[0] == ConfigControl::ReceiveStart) {
      memcpy(&_toReceive, (void*)&data[1], sizeof(uint32_t));
      rxBuffer.clear(); 
      _received = 0;
      ESP_LOGD(CONFIG_SERVICE_TAG, "Profile receive started. Expecting %d bytes", _toReceive);
    }
  }
  else if (uuid.compare(configRxCharacteristicUUID) == 0) {
    if (_received < _toReceive) {
      rxBuffer.append(received);
      _received += received.length();
      ESP_LOGD(CONFIG_SERVICE_TAG, "Received %d bytes", _received);

      if (_received == _toReceive)
        processCommand(rxBuffer);
    }
    else
      ESP_LOGW(CONFIG_SERVICE_TAG, "Exceeded expected bytes %d/%d", _toReceive, _received);
  }
}

void ConfigService::processCommand(std::string data) {
  ESP_LOGD(CONFIG_SERVICE_TAG, "Parsing command: %s", data.c_str());
  std::size_t command_location = data.find_first_of(":");
  if (command_location == std::string::npos)
    ESP_LOGW(CONFIG_SERVICE_TAG, "Invalid command (no key)");
  else {
    std::string key = data.substr(0, command_location);
    std::string value = data.substr(command_location + 1);

    if (key == "raw") {
      ESP_LOGV(CONFIG_SERVICE_TAG, "Raw configuration received");
      _config->saveUserConfig(value, true);
    }
    else if (key == "name") {
      // limit to 100 characters
      std::string name = value.substr(0, std::min((int) value.length(), 100));
      AmpStorage::saveDeviceName(name);
      BluetoothLE::instance()->updateAdvertising(name, true);
    }
    else if (key == "effect" || key == "saveEffect") {
      bool save = key == "saveEffect";
      size_t actionLocation = value.find_first_of(",");
      std::string action = value.substr(0, actionLocation);
      std::string regionString = value.substr(actionLocation + 1);
      size_t regionLocation = regionString.find_first_of(",");
      std::string region = regionString.substr(0, regionLocation);
      std::string effect = regionString.substr(regionLocation + 1);

      bool valid = _config->addEffect(action, region, effect, save);
      if (valid) {
        ESP_LOGI(CONFIG_SERVICE_TAG, "Effect received - action: %s region: %s effect: %s",
          action.c_str(), region.c_str(), effect.c_str());
        
        if (save)
          _config->saveConfig();
      }
      else
        ESP_LOGW(CONFIG_SERVICE_TAG, "Invalid effect received - action: %s region: %s effect: %s",
          action.c_str(), region.c_str(), effect.c_str());
    }
    else if (key == "removeEffect") {
      // size_t actionLocation = value.find_first_of(",");
      // std::string action = value.substr(0, actionLocation);
      // std::string region = value.substr(actionLocation + 1);

      // _config->removeEffect(action, region, true);
      // _config->saveConfig();
    }
    else if (key == "get") {
      if (value == "config") {
        ESP_LOGD(CONFIG_SERVICE_TAG, "Config requested");
        std::string data = std::string("raw:").append(_config->getRawConfig());
        uint32_t length = data.length();
        uint8_t raw[5];
        raw[0] = ConfigControl::TransmitStart;
        memcpy(&raw[1], &length, sizeof(uint32_t));
        _configStatusCharacteristic->setValue(raw);
        _configStatusCharacteristic->notify(true);
        transmit(data);
      }
    }
    else if (key == "save")
      _config->saveConfig();
  }
}

std::vector<std::string> ConfigService::buildPackets(std::string data, size_t packetSize) {
  // calculate how many packets
  size_t packetCount = data.length() / packetSize;
  packetCount = data.length() % packetSize == 0 ? packetCount : packetCount + 1;

  std::vector<std::string> packets;
  // construct each packet
  for (int i = 0; i < packetCount; i++) {
    // start of packet
    std::string packet = i == packetCount - 1 ? data.substr(i * packetSize) : data.substr(i * packetSize, packetSize);
    // add to vector of packets
    packets.push_back(packet);
  }

  return packets;
}

void ConfigService::transmit(std::string data) {
  size_t chars = data.length();
  auto m_properties = _configTxCharacteristic->m_properties;
  auto m_subscribedVec = _configTxCharacteristic->m_subscribedVec;
  bool is_notification = true;

  if (m_subscribedVec.size() == 0) {
    return;
  }

  _configTxCharacteristic->m_pCallbacks->onNotify(_configTxCharacteristic);

  bool reqSec = (m_properties & BLE_GATT_CHR_F_READ_AUTHEN) ||
                (m_properties & BLE_GATT_CHR_F_READ_AUTHOR) ||
                (m_properties & BLE_GATT_CHR_F_READ_ENC);
  int rc = 0;

  for (auto &it : m_subscribedVec) {
    uint16_t _mtu = _configTxCharacteristic->getService()->getServer()->getPeerMTU(it.first);
    uint16_t packetSize = _mtu - 3;

    // check if connected and subscribed
    if(_mtu == 0 || it.second == 0)
      continue;    

    // check if security requirements are satisfied
    if(reqSec) {
      struct ble_gap_conn_desc desc;
      rc = ble_gap_conn_find(it.first, &desc);
      if(rc != 0 || !desc.sec_state.encrypted)
        continue;
    }

    if(!(it.second & 0x0001))
      is_notification = false;

    if(!(it.second & 0x0002))
      is_notification = true;

    // notify that we're starting a transmission
    configTransceiver.wait("config");
    configTransceiver.take("config");

    // send packets
    auto packets = buildPackets(data, packetSize);
    for (auto packet : packets) {
      notify(it.first, packet, is_notification);
      delay(10);
    }
    
    configTransceiver.give();
  }
}

void ConfigService::notify(uint16_t conn_id, std::string value, bool is_notification) {
  int rc = 0;
  auto m_properties = _configTxCharacteristic->m_properties;
  auto m_handle = _configTxCharacteristic->m_handle;

  // don't create the m_buf until we are sure to send the data or else
  // we could be allocating a buffer that doesn't get released.
  // We also must create it in each loop iteration because it is consumed with each host call.
  os_mbuf *om = ble_hs_mbuf_from_flat((uint8_t*)value.data(), value.length());

  NimBLECharacteristicCallbacks::Status statusRC;

  if(!is_notification && (m_properties & NIMBLE_PROPERTY::INDICATE)) {
    ble_task_data_t taskData = {nullptr, xTaskGetCurrentTaskHandle(),0, nullptr};
    _configTxCharacteristic->m_pTaskData = &taskData;

    rc = ble_gattc_indicate_custom(conn_id, m_handle, om);
    if (rc != 0)
      statusRC = NimBLECharacteristicCallbacks::Status::ERROR_GATT;
    else {
      ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
      rc = _configTxCharacteristic->m_pTaskData->rc;
    }

    _configTxCharacteristic->m_pTaskData = nullptr;

    if (rc == BLE_HS_EDONE) {
      rc = 0;
      statusRC = NimBLECharacteristicCallbacks::Status::SUCCESS_INDICATE;
    } 
    else if (rc == BLE_HS_ETIMEOUT)
      statusRC = NimBLECharacteristicCallbacks::Status::ERROR_INDICATE_TIMEOUT;
    else
      statusRC = NimBLECharacteristicCallbacks::Status::ERROR_INDICATE_FAILURE;
  } 
  else {
    rc = ble_gattc_notify_custom(conn_id, m_handle, om);
    if (rc == 0)
      statusRC = NimBLECharacteristicCallbacks::Status::SUCCESS_NOTIFY;
    else
      statusRC = NimBLECharacteristicCallbacks::Status::ERROR_GATT;
  }

  _configTxCharacteristic->m_pCallbacks->onStatus(_configTxCharacteristic, statusRC, rc);
}