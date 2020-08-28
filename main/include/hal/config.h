#pragma once
#include <common.h>
#include <string>
#include <vector>
#include <map>

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

class Config : public LifecycleBase {
  std::vector<ConfigListener*> configListeners;
  AmpStorage ampStorage;

  StaticJsonDocument<5000> document;
  std::string rawConfig;
  std::string renderer;

  bool _filesystemError = false;
  bool _valid = false;

  std::string configPath = "/spiffs/config.mp";

  public:
    static AmpConfig ampConfig;
    void onPowerUp();
    void onPowerDown();

    void process();

    static DeviceInfo getDeviceInfo();
    bool loadConfigFile();
    bool loadConfig(std::string msgPackData);
    void saveConfig();

    void loadPrefsConfig(JsonObject prefsJson);
    void loadMotionConfig(JsonObject motionJson);
    void loadLightsConfig(JsonObject lightsJson);
    std::string readFile(std::string filename);
    void addConfigListener(ConfigListener *listener);

    bool isValid() { return _valid; }
    std::string getRawConfig() { return rawConfig; }
};