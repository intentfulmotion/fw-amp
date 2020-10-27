#include <hal/ble.h>

FreeRTOS::Semaphore BluetoothLE::bleReady = FreeRTOS::Semaphore("ble");

BluetoothLE::BluetoothLE() {
  touchEventQueue = xQueueCreate(10, sizeof(std::vector<TouchType>));
  bleReady.take();
}

void BluetoothLE::onPowerUp() {
  xTaskCreatePinnedToCore(startServer, "ble-server", 4096, NULL, 3, &bleTaskHandle, 0);
}

void BluetoothLE::onPowerDown() {
  // BLEDevice::deinit(true);

  vTaskDelete(bleTaskHandle);
}

void BluetoothLE::process() {
  if (uxQueueMessagesWaiting(touchEventQueue)) {
    xQueueReceive(touchEventQueue, &touches, 0);

    onTouchEvent(&touches);
  }

  if (publicAdvertising && millis() - publicAdvertiseStart >= PUBLIC_ADVERTISEMENT_MS) {
    notifyListeners(false);
    updateAdvertising(AmpStorage::getDeviceName(), false);
  }
}

void BluetoothLE::startServer(void *params) {
  auto ble = BluetoothLE::instance();

  NimBLEDevice::setSecurityAuth(true, true, true);
  NimBLEDevice::setSecurityCallbacks(ble);
  NimBLEDevice::setSecurityInitKey(BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID);
  NimBLEDevice::setSecurityRespKey(BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID);
  NimBLEDevice::init("");
  NimBLEDevice::setMTU(512);

  ble->server = NimBLEDevice::createServer();
  ble->advertising = ble->server->getAdvertising();
  ble->server->start();
  ble->server->setCallbacks(ble, true);
  bleReady.give();

  for (;;) {
    ble->process();
    delay(1000);
  }
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

  updateAdvertising(AmpStorage::getDeviceName(), false);
}

void BluetoothLE::updateAdvertising(std::string name, bool publicAdvertise) {
  advertising->stop();

  ESP_LOGD(BLE_TAG,"Updating advertising - name: %s, %s", name.c_str(), publicAdvertise ? "public" : "private");
  NimBLEAdvertisementData data;
  data.setCompleteServices(NimBLEUUID::fromString(vehicleServiceUUID));
  data.setFlags(BLE_HS_ADV_F_DISC_GEN);
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
    updateAdvertising(AmpStorage::getDeviceName(), true);
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