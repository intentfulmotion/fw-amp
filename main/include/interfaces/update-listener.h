#pragma once
#include <models/update-status.h>
#include "FreeRTOS.h"

class UpdateListener {
  protected:
    UpdateStatus _updateStatus;
    
  public:
    QueueHandle_t updateStatusQueue = NULL;

    virtual void onUpdateStatusChanged(UpdateStatus status) = 0;
};