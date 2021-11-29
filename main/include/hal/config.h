#pragma once
#include <common.h>
#include <string>
#include <vector>
#include <map>
#include <algorithm>

#define ARDUINOJSON_ENABLE_STD_STRING 1
#include <ArduinoJson.h>

#include <models/light.h>
#include <models/motion.h>
#include <models/config.h>
#include <interfaces/config-listener.h>
#include <interfaces/lifecycle.h>
#include <hal/common/amp-storage.h>

static const char *CONFIG_TAG = "config";

struct MsgPackFileWriter
{
  FILE *_file;

  size_t write(uint8_t c) { return fputc(c, _file); }
  size_t write(const uint8_t *buffer, size_t length) { return fwrite(buffer, sizeof(uint8_t), length, _file); }

  MsgPackFileWriter(FILE *file) : _file(file) {}
};

struct MsgPackFileReader
{
  FILE *_file;

  int read() { return fgetc(_file); }
  size_t readBytes(char *buffer, size_t length) { return fread((void *)buffer, sizeof(uint8_t), length, _file); }

  MsgPackFileReader(FILE *file) : _file(file) {}
};

class Config : public LifecycleBase
{
  std::vector<ConfigListener *> configListeners;
  AmpStorage ampStorage;

  std::string rawConfig;
  std::string renderer;

  bool _filesystemError = false;
  bool _valid = false;
  bool _isUserConfig = false;

  const std::string configPath = "/spiffs/config.mp";
  const std::string userConfigPath = "/spiffs/config.user.mp";
  const std::string forwardSuffix = "-forward";
  const std::string backwardSuffix = "-backward";
  size_t forwardSuffixLength, backwardSuffixLength;

  JsonObject serializeEffects();
  static bool parseEffect(std::string data, LightingParameters *params);
  static ColorOption parseColorOption(std::string data);

  StaticJsonDocument<10000> document;

  bool loadConfigFile(std::string path);

public:
  Config()
  {
    forwardSuffixLength = forwardSuffix.length();
    backwardSuffixLength = backwardSuffix.length();
  }
  static AmpConfig ampConfig;
  void onPowerUp();
  void onPowerDown();

  void process();

  static DeviceInfo getDeviceInfo();

  void loadConfig();
  void saveConfig();
  void saveUserConfig(std::string data, bool load = false);

  void loadActionConfig(JsonObject actionJson);
  void loadMotionConfig(JsonObject motionJson);
  void loadLightsConfig(JsonObject lightsJson);
  void loadBatteryConfig(JsonObject batteryJson);
  void loadBalanceConfig(JsonObject balanceJson);
  std::string readFile(std::string filename);
  void addConfigListener(ConfigListener *listener);
  void notifyConfigListeners();

  void updateDeviceName(std::string name);
  bool addEffect(std::string action, std::string region, std::string data, bool updateJson = false);
  // void removeEffect(std::string, std::string region, bool updateJson = false);
  std::vector<LightingParameters> *getActionEffects(std::string action);

  bool isValid() { return _valid; }
  std::string getRawConfig()
  {
    std::string data;
    serializeMsgPack(document, data);
    return data;
  }

  static FreeRTOS::Semaphore effectsUpdating;
};