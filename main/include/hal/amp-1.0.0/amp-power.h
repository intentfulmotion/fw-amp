#pragma once

#include <common.h>
#include <algorithm>
#include <driver/adc.h>
#include <esp_sleep.h>
#include "driver/gpio.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include <esp_bt.h>

#if defined(OTA_ENABLED)
#include <WiFi.h>
#endif

class AmpPower {
  // filter co-efficient
  const float alpha = 0.999;
  esp_adc_cal_characteristics_t *adc_chars;  
  float batteryReading;
  
  public:
    bool charging;
    bool done;
    bool batteryPresent;
    uint8_t batteryLevel;
    uint32_t batteryAdcReading;
    void init();
    void deinit();
    void process();
};