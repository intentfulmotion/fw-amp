#pragma once
#include <stdint.h>

enum PowerLevel : uint8_t {
  Unknown = 0x00,
  Critical,
  Low,
  Normal,
  Charged
};

struct PowerStatus {
  bool charging;
  bool doneCharging;
  bool batteryPresent;
  uint8_t percentage;
  enum PowerLevel level;
};

inline bool operator==(const PowerStatus& lhs, const PowerStatus& rhs) {
  return lhs.charging == rhs.charging && lhs.batteryPresent == rhs.batteryPresent && abs(lhs.percentage - rhs.percentage) > 5.0;
}

inline bool operator!=(const PowerStatus& lhs, const PowerStatus& rhs) {
  return lhs.charging != rhs.charging || lhs.batteryPresent != rhs.batteryPresent || abs(lhs.percentage - rhs.percentage) > 5.0;
}