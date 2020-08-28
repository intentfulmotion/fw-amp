#pragma once
#include <models/config.h>
#include "FreeRTOS.h"

class ConfigListener {
  public:
    QueueHandle_t configUpdatedQueue = NULL;
    virtual void onConfigUpdated() = 0;
};