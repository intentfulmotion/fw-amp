#pragma once

#include <common.h>
#include <algorithm>
#include <esp_bt.h>

#if defined(OTA_ENABLED)
#include <WiFi.h>
#endif

class AmpPower {
  float batteryReading;
  
  public:
    bool charging;
    bool done;
    bool batteryPresent;
    uint8_t batteryLevel;
    uint32_t batteryAdcReading;
    void init() { }
    void deinit() { }
    void process() { }
};