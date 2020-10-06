#pragma once
#include <models/light.h>

class RenderHost {    
  public:
    virtual void setBrakes(LightCommand command) = 0;
    virtual void setHeadlight(LightCommand command) = 0;
    virtual void setTurnLights(LightCommand command) = 0;
};

class RenderListener {
  public:
    QueueHandle_t lightsChangedQueue = NULL;
    virtual void onLightsChanged(LightCommands commands) = 0;
};