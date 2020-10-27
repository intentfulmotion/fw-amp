#pragma once
#include <models/light.h>

class RenderHost {    
  public:
    virtual void setMotion(Actions command) = 0;
    virtual void setHeadlight(Actions command) = 0;
    virtual void setTurnLights(Actions command) = 0;
    virtual void setOrientationLights(Actions command) = 0;
};

class RenderListener {
  public:
    QueueHandle_t lightsChangedQueue = NULL;
    virtual void onLightsChanged(LightCommands commands) = 0;
};