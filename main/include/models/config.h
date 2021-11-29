#pragma once
#include <common.h>
#include <string>
#include <map>
#include <vector>
#include <models/light.h>
#include <models/motion.h>
#include <models/battery.h>
#include <models/balance.h>

struct DeviceInfo
{
  std::string manufacturer = "Intentful Motion, Inc.";
  std::string hardwareVersion;
  std::string serialNumber;
  std::string firmwareVersion = FIRMWARE_VERSION;
  std::string deviceName;
};

struct AmpConfig
{
  MotionConfig motion;
  LightsConfig lights;
  BatteryConfig battery;
  BalanceConfig balance;
  std::map<std::string, std::vector<LightingParameters> *> actions;
  DeviceInfo info;
};

enum ConfigControl : uint8_t
{
  ReceiveStart = 0x01,
  TransmitStart
};