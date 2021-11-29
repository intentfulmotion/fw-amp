#pragma once

#include <stdint.h>
#include <vector>
#include <string>
#include <map>

#define NO_LIMIT -1

enum StripType : uint8_t
{
  NeoPixel_GRB = 0,
  NeoPixel_GRBW,
  NeoPixel_RGB,
  NeoPixel_RGBW,
  DotStar_BGR,
  DotStar_LBGR,
  DotStar_GRB,
  DotStar_LGRB
};

struct LightChannel
{
  uint8_t channel;
  uint16_t leds;
  LEDType type;
};

struct LightSection
{
  uint8_t channel;
  uint16_t start;
  uint16_t end;
};

struct LightRegion
{
  std::string name;
  std::vector<LightSection> sections;
  uint32_t count;
  std::vector<uint16_t> breaks;
};

struct LightsConfig
{
  std::map<std::string, LightRegion> regions;
  std::map<uint8_t, LightChannel> channels;
};

enum LightEffect : uint8_t
{
  Transparent = 0x00,
  Off,
  Static,
  Blink,
  Alternate,
  ColorWipe,
  Breathe,
  Fade,
  Scan,
  Rainbow,
  RainbowCycle,
  ColorChase,
  TheaterChase,
  Twinkle,
  Sparkle,
  Battery
};

enum Actions
{
  NoCommand = 0x00,
  LightsOff,
  LightsReset,
  LightsMotionNeutral,
  LightsMotionBrakes,
  LightsMotionAcceleration,
  LightsHeadlightNormal,
  LightsHeadlightBright,
  LightsTurnCenter,
  LightsTurnLeft,
  LightsTurnRight,
  LightsTurnHazard,
  LightsOrientationUnknown,
  LightsOrientationTop,
  LightsOrientationBottom,
  LightsOrientationLeft,
  LightsOrientationRight,
  LightsOrientationFront,
  LightsOrientationBack,
  LightsBalanceNoFault,
  LightsBalanceLeftFault,
  LightsBalanceRightFault,
  LightsBalanceFullFault,
  LightsBatteryCritical,
  LightsBatteryLow,
  LightsBatteryNormal
};

struct LightCommands
{
  Actions motionCommand;
  Actions headlightCommand;
  Actions turnCommand;
  Actions orientationCommand;
};

struct ColorOption
{
  Color color;
  bool random;
  bool rainbow;
};

struct LightingParameters
{
  std::string region;
  LightEffect effect;
  LightEffect otherEffect;
  uint8_t layer;
  ColorOption first;
  ColorOption second;
  ColorOption third;
  uint32_t duration;
  int limit = NO_LIMIT;
  void *extra;

  LightingParameters &operator=(LightingParameters const &other)
  {
    region = other.region;
    effect = other.effect;
    otherEffect = other.otherEffect;
    layer = other.layer;
    first = other.first;
    second = other.second;
    third = other.third;
    duration = other.duration;
    limit = other.limit;
    extra = other.extra;
    return *this;
  }
};

struct RenderStep
{
  unsigned long step;
  unsigned long next;
  void *data;
};

inline bool operator<(const LightingParameters &lhs, const LightingParameters &rhs) { return lhs.layer < rhs.layer; }
inline bool operator>(const LightingParameters &lhs, const LightingParameters &rhs) { return lhs.layer > rhs.layer; }
inline bool operator<=(const LightingParameters &lhs, const LightingParameters &rhs) { return lhs.layer <= rhs.layer; }
inline bool operator>=(const LightingParameters &lhs, const LightingParameters &rhs) { return lhs.layer >= rhs.layer; }
inline bool operator==(const LightingParameters &lhs, const LightingParameters &rhs) { return lhs.layer == rhs.layer; }
inline bool operator!=(const LightingParameters &lhs, const LightingParameters &rhs) { return lhs.layer != rhs.layer; }