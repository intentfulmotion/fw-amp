#pragma once

#define AMP_1_0_x
#define AMP_LOG_LEVEL AMP_LOG_LEVEL_VERBOSE

#define FIRMWARE_VERSION    "1.0.0"
#define COPYRIGHT_YEAR      2020

// Status LED
#define STATUS_LED          13

// I/O Channels
#define STRIP_ONE_DATA      22
#define STRIP_ONE_CLK       21
#define STRIP_TWO_DATA      33
#define STRIP_TWO_CLK       25
#define STRIP_THREE_DATA    26
#define STRIP_THREE_CLK     27
#define STRIP_FOUR_DATA     14
#define STRIP_FOUR_CLK      12

// Input
#define BUTTON_INPUT        GPIO_NUM_36

// Power Management
#define POWER_HOLD          GPIO_NUM_32
#define VBAT_SENSE          GPIO_NUM_39
#define BAT_CHRG            GPIO_NUM_34
#define BAT_DONE            GPIO_NUM_35

// IMU
#define IMU_CLK             GPIO_NUM_18
#define IMU_MISO            GPIO_NUM_19
#define IMU_MOSI            GPIO_NUM_23
#define IMU_CS              GPIO_NUM_5
#define BLE_ENABLED

#define FASTLED_ESP32_I2S true
#include <FastLED.h>
#define Color CRGB
#define LightController CLEDController

#define IO_PIN_SELECT(x)  (1ULL<<x)

static const Color ampPink(255, 33, 125);
static const Color ampPurple(134, 23, 237);
static const Color ampOrange(240, 109, 0);
static const Color lightOff(0, 0, 0);
static const Color updateStart(0, 0, 127);
static const Color updateProgress(0, 127, 0);
static const Color updateEnd(127, 127, 0);
static const Color updateError(127, 0, 0);

#include <stdint.h>
#include <memory>
#include <iostream>
#include <string>
#include <cstdio>
#include <log.h>
#include "esp_task_wdt.h"

#define PI 3.1415926535897932384626433832795

#define BOOT_NORMAL 1
#define BOOT_OTA 2
#define BOOT_DEBUG 3

#define DEFAULT_TURN_THRESHOLD 7.0      // degrees
#define DEFAULT_BRAKE_THRESHOLD 0.2     // g

#define DEFAULT_ORIENTATION_UP_MIN 70   // degrees
#define DEFAULT_ORIENTATION_UP_MAX 110  // degrees

inline Color hexStringToColor(std::string fadeColorText) {
  size_t pos = fadeColorText.find_first_of('#');
  if (pos != std::string::npos)
    fadeColorText.erase(pos, 1);

  if (fadeColorText.length() == 6) {
    uint r; uint g; uint b;
    sscanf(fadeColorText.substr(0, 2).c_str(), "%02X", &r);
    sscanf(fadeColorText.substr(2, 4).c_str(), "%02X", &g);
    sscanf(fadeColorText.substr(4, 6).c_str(), "%02X", &b);
    ESP_LOGD("Fade Color", "%d, %d, %d", r, g, b);
    return Color(static_cast<uint8_t>(r), static_cast<uint8_t>(g), static_cast<uint8_t>(b));
  }
  else
    return lightOff;
}

template<typename ... Args>
inline std::string string_format( const std::string& format, Args ... args )
{
    size_t size = snprintf( nullptr, 0, format.c_str(), args ... ) + 1; // Extra space for '\0'
    std::unique_ptr<char[]> buf( new char[ size ] ); 
    snprintf( buf.get(), size, format.c_str(), args ... );
    return std::string( buf.get(), buf.get() + size - 1 ); // We don't want the '\0' inside
}

// power
const float voltageBreakpoints[21] = {
  4.2,
  4.15,
  4.11,
  4.08,
  4.02,
  3.98,
  3.95,
  3.91,
  3.87,
  3.85,
  3.84,
  3.82,
  3.8,
  3.79,
  3.77,
  3.75,
  3.73,
  3.71,
  3.69,
  3.61,
  3.27
};

inline uint8_t percentageFromReading(float reading) {
  for (uint8_t i = 0; i < sizeof(voltageBreakpoints); i++)
    if (reading >= voltageBreakpoints[i])
      return 100 - (5 * i);
  
  return 0;
}