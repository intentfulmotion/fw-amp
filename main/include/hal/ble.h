#pragma once
#include <interfaces/lifecycle.h>
#include <interfaces/touch-listener.h>
#include <interfaces/ble-listener.h>
#include <hal/config.h>
#include <models/config.h>
#include <constants.h>
#include <stack>
#include "FreeRTOS.h"

#if defined(AMP_1_0_x)
  #include <hal/amp-1.0.0/amp-storage.h>
  #include <NimBLEDevice.h>
  #include <NimBLEServer.h>
  #include "services/gap/ble_svc_gap.h"
#endif

#define PUBLIC_ADVERTISEMENT_MS   15000

static const char* BLE_TAG = "ble";

class BluetoothLE : public LifecycleBase, public ConfigListener, public TouchListener, public NimBLEServerCallbacks {
  NimBLEAdvertising *advertising;
  NimBLEAdvertisementData advertisementData;
  std::stack<bool> connectedDevices;
  TaskHandle_t bleTaskHandle;
  std::vector<BleListener*> listeners;
  std::vector<TouchType> touches;

  bool publicAdvertising = false;
  unsigned long publicAdvertiseStart = 0;

  public:
    NimBLEServer *server;
    BluetoothLE();

    // Lifecycle Base
    void onPowerUp();
    void onPowerDown();
    void process();

    // NimBLEServerCallbacks
    void onConnect(NimBLEServer *server) { }
    void onConnect(NimBLEServer *server, ble_gap_conn_desc *desc);
    void onDisconnect(NimBLEServer *server);
    
    // Config Listener
    void onConfigUpdated();

    // TouchListener
    void onTouchEvent(std::vector<TouchType> *touches);

    void startAdvertising();
    void updateAdvertising(std::string name, bool publicAdvertise = false);
    NimBLEService* createService(std::string uuid);

    void addAdvertisingListener(BleListener *listener);
    void notifyListeners(bool isPublic);

    static void startServer(void *params);
    static FreeRTOS::Semaphore bleReady;
};