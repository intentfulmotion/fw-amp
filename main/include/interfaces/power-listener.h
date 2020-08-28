#pragma once
#include <models/power-status.h>
#include "FreeRTOS.h"

class PowerListener {
  protected:
    PowerStatus _powerStatus;

  public:
    QueueHandle_t powerStatusQueue = NULL;
    
    virtual void onPowerStatusChanged(PowerStatus status) = 0;
};