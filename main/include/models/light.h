#pragma once

#include <stdint.h>
#include <vector>
#include <string>
#include <map>

enum StripType : uint8_t {
  NeoPixel_GRB = 0,
  NeoPixel_GRBW,
  NeoPixel_RGB,
  NeoPixel_RGBW,
  DotStar_BGR,
  DotStar_LBGR,
  DotStar_GRB,
  DotStar_LGRB
};

struct LightChannel {
  uint8_t channel;
  uint16_t leds;
  LEDType type;
};

struct LightSection {
  uint8_t channel;
  uint8_t start;
  uint8_t end;
};

struct LightRegion {
  std::string name;
  std::vector<LightSection> sections;
};

struct LightsConfig {
  std::map<std::string, LightRegion> regions;
  std::map<uint8_t, LightChannel> channels;
};

enum LightMode : uint8_t {
  NoMode = 0x00,
  RunningMode,
  LightningMode,
  TheaterChaseMode,
  TheaterChaseRainbowMode,
  RainbowMode,
};

enum LightCommand {
  NoCommand = 0x00,
  LightsOff,
  LightsReset,
  LightsRunning,
  LightsBright,
  LightsTurnCenter,
  LightsTurnLeft,
  LightsTurnRight,
  LightsTurnHazard,
  LightsBrake
};

struct LightCommands {
  LightMode mode;
  LightCommand brakeCommand;
  LightCommand turnCommand;
  LightCommand headlightCommand;
};