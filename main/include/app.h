#pragma once
#include "FreeRTOS.h"

#include <amp.h>
#include <interfaces/lifecycle.h>
#include <interfaces/motion-listener.h>
#include <interfaces/render-host.h>

#ifdef BLE_ENABLED
  #include <services/device-info-service.h>
  #include <services/battery-service.h>
  #include <services/vehicle-service.h>
  #include <services/config-service.h>
  #include <services/update-service.h>
#endif

static const char* APP_TAG = "app";

class App : public LifecycleBase, public ConfigListener, public MotionListener, public RenderHost {
  TaskHandle_t *renderHostHandle = NULL;
  static Amp *amp;
  AmpConfig *config;
  VehicleState vehicleState;
  bool _renderHostActive = false;
  std::vector<RenderListener*> renderListeners;

  Actions _motionCommand = Actions::LightsMotionNeutral;
  Actions _headlightCommand = Actions::LightsHeadlightNormal;
  Actions _turnCommand = Actions::LightsTurnCenter;
  Actions _orientationCommand = Actions::LightsOrientationUnknown;

#if defined(BLE_ENABLED)
  // services
  DeviceInfoService *deviceInfoService;
  BatteryService *batteryService;
  VehicleService *vehicleService;
  ConfigService *configService;
  UpdateService *updateService;
#endif
  
  public:
    App(Amp *instance);
    void onPowerUp();
    void onPowerDown();
    void process();
    void onConfigUpdated();
    // void setLightMode(LightMode mode);
    void addRenderListener(RenderListener* listener) { renderListeners.push_back(listener); }

    void onAccelerationStateChanged(AccelerationState state);
    void onTurnStateChanged(TurnState state);
    void onOrientationChanged(Orientation state);

    void setHeadlight(Actions command);
    void setMotion(Actions command);
    void setTurnLights(Actions command);
    void setOrientationLights(Actions command);
    void notifyLightsChanged(Actions motionCommand = NoCommand, Actions turnCommand = NoCommand, Actions headlightCommand = NoCommand, Actions orientationCommand = NoCommand);
    
    static void startRenderHost(void *parameters);
};