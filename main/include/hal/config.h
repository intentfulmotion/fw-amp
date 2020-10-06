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

#if defined(AMP_1_0_x)
  #include <hal/amp-1.0.0/amp-storage.h>
#endif

static const char* CONFIG_TAG = "config";

struct MsgPackFileWriter {
  FILE *_file;

  size_t write(uint8_t c) { return fputc(c, _file); }
  size_t write(const uint8_t *buffer, size_t length) { return fwrite(buffer, sizeof(uint8_t), length, _file); }

  MsgPackFileWriter(FILE *file) : _file(file) { }
};

struct MsgPackFileReader {
  FILE *_file;

  int read() { return fgetc(_file); }
  size_t readBytes(char* buffer, size_t length) { return fread((void*) buffer, sizeof(uint8_t), length, _file); }

  MsgPackFileReader(FILE *file) : _file(file) { }
};

class Config : public LifecycleBase {
  std::vector<ConfigListener*> configListeners;
  AmpStorage ampStorage;
  
  std::string rawConfig;
  std::string renderer;

  bool _filesystemError = false;
  bool _valid = false;

  std::string configPath = "/spiffs/config.mp";

  JsonObject serializeEffects();
  static bool parseEffect(std::string data, LightingParameters *params);
  static ColorOption parseColorOption(std::string data);

  public:
    static AmpConfig ampConfig;
    void onPowerUp();
    void onPowerDown();

    void process();

    static DeviceInfo getDeviceInfo();
    bool loadConfigFile();
    bool loadConfig(std::string msgPackData);
    void saveConfig();

    void loadActionConfig(JsonObject actionJson);
    void loadMotionConfig(JsonObject motionJson);
    void loadLightsConfig(JsonObject lightsJson);
    std::string readFile(std::string filename);
    void addConfigListener(ConfigListener *listener);
    void notifyConfigListeners();

    void updateDeviceName(std::string name);
    bool addEffect(std::string action, std::string region, std::string data, bool updateJson = false);
    // void removeEffect(std::string, std::string region, bool updateJson = false);
    std::vector<LightingParameters>* getActionEffects(std::string action);

    bool isValid() { return _valid; }
    std::string getRawConfig() { return rawConfig; }

    static FreeRTOS::Semaphore effectsUpdating;
};