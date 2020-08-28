#pragma once
#include <models/light.h>

class RenderHost {
  protected:
    LightMode lightMode;
  public:
    QueueHandle_t lightModeQueue = NULL;
    virtual void setLightMode(LightMode mode);
    virtual void setBrakes(LightCommand command);
    virtual void setHeadlight(LightCommand command);
    virtual void setTurnLights(LightCommand command);
    LightMode getLightMode() { return lightMode; }
};

class RenderListener {
  public:
    QueueHandle_t lightsChangedQueue = NULL;
    virtual void onLightsChanged(LightCommands commands);
};