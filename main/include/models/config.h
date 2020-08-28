#pragma once
#include <string>
#include <models/light.h>
#include <models/motion.h>

struct DeviceInfo {
  std::string manufacturer = "Intentful Motion, Inc.";
  std::string hardwareVersion;
  std::string serialNumber;
  std::string firmwareVersion = FIRMWARE_VERSION;
};

struct PrefsConfig {
  std::string deviceName;
  long turnFlashRate;
  long brakeFlashRate;
  long brakeDuration;
  LightMode renderer;
};

struct AmpConfig {
  MotionConfig motion;
  LightsConfig lights;
  PrefsConfig prefs;
  DeviceInfo info;
};