#pragma once

// #define AMP_1_0_x
#define AMP_PINT

#define FIRMWARE_VERSION    "2.0.0"
#define COPYRIGHT_YEAR      2021

#ifdef AMP_1_0_x
#include "hal/amp-1.0.0/board.h"
#endif

#ifdef AMP_PINT
#include "hal/pint/board.h"
#endif

#include "AddressableLED.h"
#define Color Rgb
#define LightController AddressableLED

#define IO_PIN_SELECT(x)  (1ULL<<x)

static const Color ampPink(255, 33, 125);
static const Color ampPurple(134, 23, 237);
static const Color ampOrange(240, 109, 0);
static const Color lightOff(0, 0, 0);
static const Color updateStart(0, 0, 127);
static const Color updateProgress(0, 127, 0);
static const Color updateEnd(127, 127, 0);
static const Color updateError(127, 0, 0);

#include <FreeRTOS.h>
#include <memory>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <cstdio>
#include "esp_task_wdt.h"

#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG
#include <esp_log.h>

#define PI 3.1415926535897932384626433832795

#define BOOT_NORMAL 1
#define BOOT_OTA 2
#define BOOT_DEBUG 3

#define DEFAULT_TURN_THRESHOLD 7.0      // degrees
#define DEFAULT_BRAKE_THRESHOLD 0.2     // g
#define DEFAULT_ACCELERATION_THRESHOLD 0.2     // g

#define DEFAULT_ORIENTATION_UP_MIN 70   // degrees
#define DEFAULT_ORIENTATION_UP_MAX 110  // degrees

#define DEFAULT_EMPTY_BATTERY 3.0
#define DEFAULT_LOW_BATTERY 3.1
#define DEFAULT_FULL_BATTERY 4.2

inline Color hexStringToColor(std::string fadeColorText) {
  size_t pos = fadeColorText.find_first_of('#');
  if (pos != std::string::npos)
    fadeColorText.erase(pos, 1);

  if (fadeColorText.length() == 6) {
    uint r; uint g; uint b;
    sscanf(fadeColorText.substr(0, 2).c_str(), "%02X", &r);
    sscanf(fadeColorText.substr(2, 4).c_str(), "%02X", &g);
    sscanf(fadeColorText.substr(4, 6).c_str(), "%02X", &b);
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

inline bool hasEnding (std::string const &fullString, std::string const &ending) {
    if (fullString.length() >= ending.length()) {
        return (0 == fullString.compare (fullString.length() - ending.length(), ending.length(), ending));
    } else {
        return false;
    }
}

inline unsigned long micros()
{
  return (unsigned long) (esp_timer_get_time());
}

inline unsigned long millis()
{
  return (unsigned long) (esp_timer_get_time() / 1000ULL);
}

inline void delay(uint32_t ms) {
  vTaskDelay(ms / portTICK_PERIOD_MS);
}

// power
const int VOLTAGE_BREAKPOINTS = 21;
const float DEFAULT_VOLTAGE_BREAKPOINTS[VOLTAGE_BREAKPOINTS] = {
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

inline uint8_t percentageFromReading(float reading, float voltageBreakpoints[]) {
  float x0, y0, x1, y1, interpolated;
  for (uint8_t i = 0; i < VOLTAGE_BREAKPOINTS; i++)
    if (reading >= voltageBreakpoints[i]) {
      if (i == 0)
        return 100.0;
      else {
        x0 = voltageBreakpoints[i - 1];
        y0 = 100 - (5 * (i - 1));
        x1 = voltageBreakpoints[i];
        y1 = 100 - (5 * i);
        interpolated = y0 + ((y1 - y0) / (x1 - x0)) * (reading - x0);
        return (int) interpolated;
      }
    }
  
  return 0;
}

inline Color hexToColor(std::string hex) {
  Color color;
  sscanf(hex.c_str(), "#%02x%02x%02x", (uint*) &color.r, (uint*) &color.g, (uint*) &color.b);

  return color;
}

inline std::vector<std::string> split(const std::string &s, char delim) {
  std::stringstream ss(s);
  std::string item;
  std::vector<std::string> elems;
  while (std::getline(ss, item, delim))
    elems.push_back(std::move(item));

  return elems;
}