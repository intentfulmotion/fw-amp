#pragma once
#include <NimBLEService.h>
#include <hal/ble.h>

#include <constants.h>
#include <hal/config.h>

class DeviceInfoService {
  NimBLEServer *_server;
  NimBLECharacteristic* _hardwareVersionCharacteristic;
  NimBLECharacteristic* _firmwareVersionCharacteristic;
  NimBLECharacteristic* _manufacturerCharacteristic;
  NimBLECharacteristic* _serialNumberCharacteristic;

  public:
    DeviceInfoService(BLEServer *server);

    void setupService();
    void onConfigUpdated();
};