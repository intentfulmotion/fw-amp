#pragma once
#include "FreeRTOS.h"

#include <amp.h>
#include <interfaces/renderer.h>
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

#include <renderers/running.h>
#include <renderers/pattern.h>

class App : public LifecycleBase, public ConfigListener, public MotionListener, public RenderHost {
  TaskHandle_t *renderHostHandle = NULL;
  static Amp *amp;
  AmpConfig *config;
  VehicleState vehicleState;
  bool _renderHostActive = false;
  Renderer *renderer = NULL;
  std::vector<RenderListener*> renderListeners;

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
    void setLightMode(LightMode mode);
    void addRenderListener(RenderListener* listener) { renderListeners.push_back(listener); }

    void onAccelerationStateChanged(AccelerationState state);
    void onTurnStateChanged(TurnState state);
    void onOrientationChanged(Orientation state);

    void setHeadlight(LightCommand command);
    void setBrakes(LightCommand command);
    void setTurnLights(LightCommand command);
    void notifyLightsChanged(LightCommand brakeCommand = NoCommand, LightCommand turnCommand = NoCommand, LightCommand headlightCommand = NoCommand);
    
    static void startRenderHost(void *parameters);
};