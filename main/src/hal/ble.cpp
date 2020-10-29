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
  vTaskDelete(bleTaskHandle);
}

void BluetoothLE::process() {
  if (uxQueueMessagesWaiting(touchEventQueue)) {
    xQueueReceive(touchEventQueue, &touches, 0);

    onTouchEvent(&touches);
  }

  if (publicAdvertising && millis() - publicAdvertiseStart >= PUBLIC_ADVERTISEMENT_MS) {
    notifyListeners(false);
    // updateAdvertising(AmpStorage::getDeviceName(), false);
    // advertising->stop();
    NimBLEDevice::setSecurityAuth(false, true, true);
  }
}

void BluetoothLE::startServer(void *params) {
  auto ble = BluetoothLE::instance();

  NimBLEDevice::init("");
  NimBLEDevice::setSecurityAuth(false, true, true);
  NimBLEDevice::setSecurityInitKey(BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID);
  NimBLEDevice::setSecurityRespKey(BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID);
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

void BluetoothLE::onAuthenticationComplete(ble_gap_conn_desc *conn) {
  ESP_LOGD(BLE_TAG, "Authentication Complete - bonded: %s authenticated: %s", conn->sec_state.bonded ? "yes" : "no", conn->sec_state.authenticated ? "yes" : "no");
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
  // if we're not publicly advertising, let's see if we've previously bonded
  // if not, disconnect the device
  auto addr = desc->peer_id_addr.val;
  ESP_LOGD(BLE_TAG, "Connected to device %02X:%02X:%02X:%02X:%02X:%02X - bonded: %s", addr[0], addr[1], addr[2], addr[3], addr[4], addr[5], desc->sec_state.bonded ? "yes" : "no");

  if (!publicAdvertising) {
    ble_addr_t bonded[MYNEWT_VAL(BLE_STORE_MAX_BONDS)];
    int numPeers;
    ble_store_util_bonded_peers(&bonded[0], &numPeers, MYNEWT_VAL(BLE_STORE_MAX_BONDS));

    bool found = false;
    for (int i = 0; i < numPeers; i++) {
      if (ble_addr_cmp(&desc->peer_id_addr, &bonded[i]) == 0) {
        found = true;
        break;
      }
    }

    if (!found) {
      auto addr = desc->peer_id_addr.val;
      ESP_LOGD(BLE_TAG, "Not bonded to device %02X:%02X:%02X:%02X:%02X:%02X - disconnecting", addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);
      server->disconnect(desc->conn_handle);
    }
  }

  advertising->start();
}

void BluetoothLE::onDisconnect(NimBLEServer *server) {
  advertising->start();
}

void BluetoothLE::onTouchEvent(std::vector<TouchType> *touches) {
  if (touches->size() == 3) {
    notifyListeners(true);
    updateAdvertising(AmpStorage::getDeviceName(), true);
    NimBLEDevice::setSecurityAuth(true, true, true);
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