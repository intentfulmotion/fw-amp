#pragma once
#include "FreeRTOS.h"

class CalibrationListener {
  public:
    QueueHandle_t calibrateXGQueue = NULL;
    QueueHandle_t calibrateMagQueue = NULL;
    
    virtual void onCalibrateXGStarted() = 0;
    virtual void onCalibrateXGEnded() = 0;

    virtual void onCalibrateMagStarted() = 0;
    virtual void onCalibrateMagEnded() = 0;
};