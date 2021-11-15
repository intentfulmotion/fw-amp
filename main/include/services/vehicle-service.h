#pragma once
#include <NimBLEService.h>
#include <hal/power.h>
#include <hal/ble.h>
#include <models/control.h>
#include <constants.h>
#include <interfaces/motion-listener.h>
#include <interfaces/render-host.h>
#include <interfaces/motion-provider.h>
#include <interfaces/power-provider.h>
#include <interfaces/calibration-listener.h>

static const char* VEHICLE_SERVICE_TAG = "vehicle-service";

class VehicleService : public NimBLECharacteristicCallbacks, public MotionListener, public CalibrationListener, public RenderListener {
  MotionProvider *_motion;
  PowerProvider *_power;
  NimBLEServer *_server;
  RenderHost *_renderHost;
  NimBLECharacteristic *_controlCharacteristic;
  NimBLECharacteristic *_stateCharacteristic;
  NimBLECharacteristic *_lightCharacteristic;
  NimBLECharacteristic *_calibrationCharacteristic;
  NimBLECharacteristic *_restartCharacteristic;

  public:
    VehicleService(MotionProvider *motion, PowerProvider *power, NimBLEServer *server, RenderHost *host);

    void setupService();
    void onWrite(NimBLECharacteristic *characteristic);
    void process();
    
    void onVehicleStateChanged(VehicleState state);
    void onLightsChanged(LightCommands commands);

    void onCalibrateXGStarted();
    void onCalibrateXGEnded();
    void onCalibrateMagStarted();
    void onCalibrateMagEnded();
};