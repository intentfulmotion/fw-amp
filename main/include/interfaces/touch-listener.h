#pragma once
#include <vector>
#include <FreeRTOS.h>
#include <models/touch-type.h>

class TouchListener {
  public:
    QueueHandle_t touchQueue = NULL;
    QueueHandle_t touchEventQueue = NULL;
};

class TouchTaskListener {
  public:
    virtual void onTouchDown() = 0;
    virtual void onTouchUp() = 0;
};