#include <hal/ble.h>

FreeRTOS::Semaphore BluetoothLE::bleReady = FreeRTOS::Semaphore("ble");

BluetoothLE::BluetoothLE() {
  configUpdatedQueue = xQueueCreate(1, sizeof(bool));
  touchEventQueue = xQueueCreate(10, sizeof(std::vector<TouchType>));
  bleReady.take();
}

void BluetoothLE::onPowerUp() {
  xTaskCreatePinnedToCore(startServer, "ble-server", 4096, this, 3, &bleTaskHandle, 0);
}

void BluetoothLE::onPowerDown() {
  // BLEDevice::deinit(true);

  vTaskDelete(bleTaskHandle);
}

void BluetoothLE::process() {
  bool valid;

  if (uxQueueMessagesWaiting(configUpdatedQueue)) {
    xQueueReceive(configUpdatedQueue, &valid, 0);    

    if (valid)
      onConfigUpdated();
  }

  if (uxQueueMessagesWaiting(touchEventQueue)) {
    xQueueReceive(touchEventQueue, &touches, 0);

    onTouchEvent(&touches);
  }

  if (publicAdvertising && millis() - publicAdvertiseStart >= PUBLIC_ADVERTISEMENT_MS) {
    notifyListeners(false);
    updateAdvertising(Config::ampConfig.info.deviceName, false);
  }
}

void BluetoothLE::startServer(void *params) {
  BluetoothLE *ble = (BluetoothLE*)params;

  NimBLEDevice::setSecurityIOCap(BLE_HS_IO_NO_INPUT_OUTPUT);
  NimBLEDevice::setSecurityAuth(BLE_SM_PAIR_AUTHREQ_BOND);
  NimBLEDevice::setSecurityInitKey(BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID);
  NimBLEDevice::setSecurityRespKey(BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID);
  NimBLEDevice::init(AmpStorage::getDefaultName());
  NimBLEDevice::setMTU(527);

  ble->server = NimBLEDevice::createServer();
  ble->advertising = ble->server->getAdvertising();
  ble->server->start();

  for (;;) {
    ble->process();
    delay(1000);
  }
}

void BluetoothLE::onConfigUpdated() {
  auto config = &Config::ampConfig;

  // update device name
  updateAdvertising(config->info.deviceName, true);
  bleReady.give();
}

NimBLEService* BluetoothLE::createService(std::string uuid) {
  return server->createService(NimBLEUUID::fromString(uuid));
}

void BluetoothLE::startAdvertising() {
  // configure advertising
  // generic outdoor sports activity
  advertising->setAppearance(5184);
  advertising->addServiceUUID(NimBLEUUID::fromString(vehicleServiceUUID));
  advertising->setScanResponse(true);
  // advertising->setScanFilter(true, true);
  advertising->start();
}

void BluetoothLE::updateAdvertising(std::string name, bool publicAdvertise) {
  advertising->stop();

  ESP_LOGD(BLE_TAG,"Updating advertising - name: %s, %s", name.c_str(), publicAdvertise ? "public" : "private");
  NimBLEAdvertisementData data;
  data.setCompleteServices(NimBLEUUID::fromString(vehicleServiceUUID));
  data.setFlags(publicAdvertise ? BLE_HS_ADV_F_DISC_GEN : BLE_HS_ADV_F_DISC_LTD);
  advertising->setAdvertisementData(data);
  data.setName(name);
  advertising->setName(name.c_str());
  advertising->setScanResponseData(data);
  // advertising->setScanFilter(!publicAdvertise, !publicAdvertise);
  advertising->start();
}

void BluetoothLE::onConnect(NimBLEServer *server, ble_gap_conn_desc *desc) {
  connectedDevices.push(true);
  advertising->start(); // change to stop at some point
}

void BluetoothLE::onDisconnect(NimBLEServer *server) {
  connectedDevices.pop();
  advertising->start();
}

void BluetoothLE::onTouchEvent(std::vector<TouchType> *touches) {
  if (touches->size() == 3) {
    notifyListeners(true);
    updateAdvertising(Config::ampConfig.info.deviceName, true);
  }
}

void BluetoothLE::addAdvertisingListener(BleListener *listener) {
  listeners.push_back(listener);
}

void BluetoothLE::notifyListeners(bool isPublic) {
  publicAdvertising = isPublic;
  publicAdvertiseStart = millis();

  for (auto listener : listeners)
    if (listener->advertisingQueue != NULL)
      xQueueSend(listener->advertisingQueue, &isPublic, 0);
}