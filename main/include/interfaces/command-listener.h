#pragma once
#include "FreeRTOS.h"

class CommandListener {
  public:
    QueueHandle_t headlightQueue;
    QueueHandle_t brakelightQueue;
    QueueHandle_t turnlightQueue;
};