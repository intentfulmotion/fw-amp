#pragma once

#include <hal/ble.h>
#include <hal/updater.h>
#include <NimBLEService.h>
#include <constants.h>

static const char* UPDATE_SERVICE_TAG = "update-service";

class UpdateService : public NimBLECharacteristicCallbacks, public UpdateListener {
  Updater *_updater;
  NimBLEServer *_server;
  NimBLECharacteristic *_updateRxCharacteristic;
  NimBLECharacteristic *_updateControlCharacteristic;
  NimBLECharacteristic *_updateStatusCharacteristic;

  public:
    UpdateService(Updater *updater, NimBLEServer *server);

    void setupService();
    void process();
    void onWrite(NimBLECharacteristic *characteristic);

    void onUpdateStatusChanged(UpdateStatus status);
};