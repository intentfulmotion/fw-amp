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
    NIMBLE_PROPERTY::WRITE_NR |
    NIMBLE_PROPERTY::NOTIFY);
  
  _configRxCharacteristic->setCallbacks(this);

  _configTxCharacteristic = service->createCharacteristic(
    NimBLEUUID::fromString(configTxCharacteristicUUID),
    NIMBLE_PROPERTY::READ |
    NIMBLE_PROPERTY::NOTIFY);

  service->start();
}

void ConfigService::onWrite(NimBLECharacteristic* characteristic) {
  std::string uuid = characteristic->getUUID().toString();
  std::string received = characteristic->getValue();
  ESP_LOGV(CONFIG_SERVICE_TAG,"Received: %s", received.c_str());

  if (uuid.compare(configRxCharacteristicUUID) == 0) {
    std::size_t partialMarker = received.find_first_of("##");
    if (partialMarker == std::string::npos) {
      // this is a single packet
      ESP_LOGV(CONFIG_SERVICE_TAG,"Single packet data: %s", received.c_str());
      processCommand(received);
    }
    else {
      uint8_t totalPackets = (uint8_t)received[2];
      uint8_t currentPacket = (uint8_t)received[3];
      ESP_LOGV(CONFIG_SERVICE_TAG,"Received partial packet %d/%d", currentPacket, totalPackets);

      // clear the buffer if we've received a new first packet
      if (currentPacket == 1)
        rxBuffer.clear();
      
      // append data to buffer
      rxBuffer.append(received.substr(4));

      // if it's the last packet
      if (currentPacket == totalPackets) {
        ESP_LOGD(CONFIG_SERVICE_TAG,"Received last partial packet. Total size: %d %s", rxBuffer.length(), rxBuffer.c_str());
        printf("Bytes: ");
        for (int i = 0; i < rxBuffer.length(); i++)
          printf("%d ", rxBuffer[i]);
        printf("\n");
        processCommand(rxBuffer);
        rxBuffer.clear();
      }
    }
  }
}

void ConfigService::processCommand(std::string data) {
  ESP_LOGD(CONFIG_SERVICE_TAG,"Parsing command: %s", data.c_str());
  std::size_t command_location = data.find_first_of(":");
  if (command_location == std::string::npos)
    ESP_LOGW(CONFIG_SERVICE_TAG,"Invalid command (no key)");
  else {
    std::string key = data.substr(0, command_location);
    std::string value = data.substr(command_location + 1);

    if (key == "raw") {
      ESP_LOGV(CONFIG_SERVICE_TAG,"Raw configuration received: %s", value.c_str());
      _config->loadConfig(value);

      // save config if valid
      if (_config->isValid())
        _config->saveConfig();
    }
    else if (key == "conf") {
      std::size_t configValueLocation = data.find_first_of("=");
      if (configValueLocation == std::string::npos)
        ESP_LOGW(CONFIG_SERVICE_TAG,"Missing '=' in configuration setting");
      else {
        std::string configKey = value.substr(0, configValueLocation);
        std::string configValue = value.substr(configValueLocation);
      }
    }
    else if (key == "get") {
      if (value == "config") {
        ESP_LOGD(CONFIG_SERVICE_TAG,"Config requested");
        std::string prefix = std::string("config:");
        std::string configData = _config->getRawConfig();
        transmit(prefix.append(configData));
      }
    }
  }
}

std::vector<std::string> ConfigService::buildPackets(std::string data, size_t packetSize) {
  // split up the packet
  size_t partialPacketSize = packetSize - 4;

  // calculate how many packets
  size_t packetCount = data.length() / partialPacketSize;
  packetCount = data.length() % partialPacketSize == 0 ? packetCount : packetCount + 1;

  std::vector<std::string> packets;
  // construct each packet
  for (int i = 0; i < packetCount; i++) {
    // start of packet
    std::string packet = "##";
    packet += (char)packetCount;
    packet += (char)(i + 1);
    std::string part = i == packetCount - 1 ? data.substr(i * partialPacketSize) : data.substr(i * partialPacketSize, partialPacketSize);
    packet.append(part);

    // add to vector of packets
    packets.push_back(packet);
  }

  return packets;
}

// void ConfigService::transmit(std::string data) {
//   size_t chars = data.length();
//   ESP_LOGD(CONFIG_SERVICE_TAG,"Transmitting %s", data.c_str());

//   for (NimBLEClient* device : NimBLEDevice::getClientList()) {
//     // get MTU of the peer device to determine packet size
//     uint16_t mtu = device->getMTU();
//     uint16_t packetSize = mtu - 3;

//     // build multiple packets if data is larger than packet size
//     if (chars > packetSize) {
//       auto packets = buildPackets(data, packetSize);
//       uint16_t count = 0;
//       for (auto packet : packets) {
//         ESP_LOGV(CONFIG_SERVICE_TAG,"Transmitting packet %d/%d: %s", ++count, packets.size(), packet.c_str());
//         notify(connection, packet, true);
//         delay(10);
//       }
//     }
//     else {
//       notify(connection, data, true);
//       delay(10);
//     }
//   }
// }

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

    if (chars > packetSize) {
      auto packets = buildPackets(data, packetSize);
      for (auto packet : packets) {
        notify(it.first, packet, is_notification);
        delay(10);
      }
    }
    else
      notify(it.first, data, is_notification);
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