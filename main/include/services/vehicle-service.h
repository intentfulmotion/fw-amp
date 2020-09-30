#pragma once
#include <NimBLEService.h>
#include <hal/motion.h>
#include <hal/power.h>
#include <hal/ble.h>
#include <models/control.h>
#include <constants.h>
#include <interfaces/motion-listener.h>
#include <interfaces/render-host.h>

static const char* VEHICLE_SERVICE_TAG = "vehicle-service";

class VehicleService : public NimBLECharacteristicCallbacks, public MotionListener, public CalibrationListener, public RenderListener {
  Motion *_motion;
  Power *_power;
  NimBLEServer *_server;
  RenderHost *_renderHost;
  NimBLECharacteristic *_controlCharacteristic;
  NimBLECharacteristic *_stateCharacteristic;
  NimBLECharacteristic *_lightCharacteristic;
  NimBLECharacteristic *_calibrationCharacteristic;
  NimBLECharacteristic *_restartCharacteristic;

  public:
    VehicleService(Motion *motion, Power *power, NimBLEServer *server, RenderHost *host);

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