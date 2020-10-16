#pragma once
#include <stdint.h>

enum UpdateStatus : uint8_t {
  Start = 0,
  End,
  Write,
  ErrorStart,
  ErrorEnd,
  ErrorWrite
};