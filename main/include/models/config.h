#pragma once
#include <string>
#include <map>
#include <vector>
#include <models/light.h>
#include <models/motion.h>

struct DeviceInfo {
  std::string manufacturer = "Intentful Motion, Inc.";
  std::string hardwareVersion;
  std::string serialNumber;
  std::string firmwareVersion = FIRMWARE_VERSION;
  std::string deviceName;
};

struct AmpConfig {
  MotionConfig motion;
  LightsConfig lights;
  std::map<std::string, std::vector<LightingParameters>*> actions;
  DeviceInfo info;
};

enum ConfigControl : uint8_t {
  ReceiveStart = 0x01,
  ReceiveStop,
  TransmitStart,
  TransmitStop
}