#pragma once
#include <models/motion.h>
#include "FreeRTOS.h"

class MotionListener {  
  public:
    QueueHandle_t vehicleQueue = NULL;
};