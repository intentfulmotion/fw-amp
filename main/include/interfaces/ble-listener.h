#pragma once
#include "FreeRTOS.h"

class BleListener {
  public:
    QueueHandle_t advertisingQueue = NULL;

    virtual void onAdvertisingStarted() = 0;
    virtual void onAdvertisingStopped() = 0;
};