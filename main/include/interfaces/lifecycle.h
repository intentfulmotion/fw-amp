#pragma once
#include "FreeRTOS.h"

class LifecycleBase {
  public:  
    virtual void onPowerUp() = 0;
    virtual void onPowerDown() = 0;

    virtual void process() = 0;
};