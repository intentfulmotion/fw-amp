#pragma once
#include <NimBLEService.h>
#include <hal/ble.h>
#include <constants.h>
#include <interfaces/power-listener.h>

class BatteryService : public PowerListener {
  NimBLEServer *_server;
  NimBLECharacteristic* _batteryCharacteristic;

  public:
    BatteryService(NimBLEServer *server);

    void setupService();
    void onPowerStatusChanged(PowerStatus status);
    void process();
};