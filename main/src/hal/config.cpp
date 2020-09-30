#include <hal/config.h>

AmpConfig Config::ampConfig;

void Config::onPowerUp() {
  if (!ampStorage.init()) {
    ESP_LOGE(CONFIG_TAG,"Unable to mount filsystem");
    _filesystemError = true;
  }

  ampConfig.info = getDeviceInfo();
  
  ESP_LOGI(CONFIG_TAG,"Amp %s", ampConfig.info.hardwareVersion.c_str());
  ESP_LOGI(CONFIG_TAG,"Firmware: %s", ampConfig.info.firmwareVersion.c_str());
  ESP_LOGI(CONFIG_TAG,"Serial Number: %s", ampConfig.info.serialNumber.c_str());
  ESP_LOGI(CONFIG_TAG,"Copyright %d %s", COPYRIGHT_YEAR, ampConfig.info.manufacturer.c_str());
  ESP_LOGI(CONFIG_TAG,"IDF version: %s", esp_get_idf_version());

  loadConfigFile();
}

void Config::onPowerDown() {
  ampStorage.deinit();
}

void Config::process() { }

void Config::addConfigListener(ConfigListener *listener) {
  configListeners.push_back(listener);
}

bool Config::loadConfigFile() {
  if (!_filesystemError) {
    rawConfig = ampStorage.readFile(configPath);
    return loadConfig(rawConfig);
  }
  else {
    ESP_LOGE(CONFIG_TAG,"Cannot load config due to error with filesystem.");
    return false;
  }
}

void Config::saveConfig() {
  auto file = ampStorage.writeFile(configPath);

  if (!file) {
    ESP_LOGE(CONFIG_TAG,"Could not open file: %s", configPath.c_str());
    return;
  }

  ESP_LOGD(CONFIG_TAG,"Writing config to file");
  char buffer[1024];
  auto size = serializeMsgPack(document.as<JsonObject>(), buffer);

  fwrite(buffer, 1, size, file);
  fclose(file);
}

bool Config::loadConfig(std::string msgPackData) {
  rawConfig = msgPackData;

  // load MessagePack document
  auto err = deserializeMsgPack(document, msgPackData.c_str());
  if (err) {
    ESP_LOGE(CONFIG_TAG,"deserializeMsgPack failed: %s", err.c_str());
    _valid = false;
    return _valid;
  }

  JsonObject configJson = document.as<JsonObject>();

  JsonObject lightsJson = configJson["lights"].as<JsonObject>();
  loadLightsConfig(lightsJson);

  JsonObject actionsJson = configJson["actions"].as<JsonObject>();
  loadActionConfig(actionsJson);

  JsonObject motionJson = configJson["motion"].as<JsonObject>();
  loadMotionConfig(motionJson);

  rawConfig = msgPackData;
  ESP_LOGD(CONFIG_TAG,"Loaded configuration");
  _valid = true;

  for (auto listener : configListeners)
    if (listener->configUpdatedQueue != NULL)
      xQueueSendToFront(listener->configUpdatedQueue, &_valid, 0);

  return _valid;
}

void Config::loadMotionConfig(JsonObject motionJson) {
  MotionConfig config;

  config.autoOrientation = motionJson["autoOrientation"] | false;
  config.autoMotion = motionJson["autoMotion"] | false;
  config.autoTurn = motionJson["autoTurn"] | false;
  config.relativeTurnZero = motionJson["relativeTurnZero"] | true;

  config.brakeThreshold = motionJson["brakeThreshold"] | DEFAULT_BRAKE_THRESHOLD;
  config.turnThreshold = motionJson["turnThreshold"] | DEFAULT_TURN_THRESHOLD;
  config.orientationUpMin = motionJson["orientationUpMin"] | DEFAULT_ORIENTATION_UP_MIN;
  config.orientationUpMax = motionJson["orientationUpMax"] | DEFAULT_ORIENTATION_UP_MAX;

  uint8_t brakeAxis = (motionJson["brakeAxis"].as<uint8_t>()) | AccelerationAxis::X_Pos;
  config.brakeAxis = (AccelerationAxis)brakeAxis;

  uint8_t turnAxis = (AttitudeAxis)(motionJson["turnAxis"].as<uint8_t>()) | AttitudeAxis::Roll;
  config.turnAxis = (AttitudeAxis)turnAxis;

  uint8_t orientationAxis = (AttitudeAxis)(motionJson["orientationAxis"].as<uint8_t>()) | AttitudeAxis::Pitch;
  config.orientationAxis = (AttitudeAxis)orientationAxis;

  ESP_LOGV(CONFIG_TAG,"auto orientation config: %s", config.autoOrientation ? "true" : "false");
  ESP_LOGV(CONFIG_TAG,"auto motion config: %s", config.autoMotion ? "true" : "false");
  ESP_LOGV(CONFIG_TAG,"auto turn config: %s", config.autoTurn ? "true" : "false");

  ESP_LOGV(CONFIG_TAG,"brake axis: %d threshold: %F", config.brakeAxis, config.brakeThreshold);
  ESP_LOGV(CONFIG_TAG,"turn axis: %d threshold: %F", config.turnAxis, config.turnThreshold);

  ampConfig.motion = config;
}

void Config::loadLightsConfig(JsonObject lightsJson) {
  LightsConfig config;

  // load light channels
  JsonArray channelsJson = lightsJson["channels"].as<JsonArray>();
  std::map<uint8_t, LightChannel> channels;
  for (auto channel : channelsJson) {
    LightChannel ch;
    ch.channel = channel["channel"].as<uint8_t>();
    ch.leds = channel["leds"].as<uint16_t>();
    ch.type = channel["type"].as<LEDType>();
    channels[ch.channel] = ch;
  }

  // load light regions
  std::map<std::string, LightRegion> regions;
  for (auto lightRegion : lightsJson["regions"].as<JsonObject>()) {
    std::string regionName = std::string(lightRegion.key().c_str());
    JsonArray sectionsJson = lightRegion.value().as<JsonArray>();

    LightRegion region;
    std::vector<LightSection> sections;
    region.count = 0;

    for (auto sect : sectionsJson) {
      LightSection section;
      JsonObject sectionJson = sect.as<JsonObject>();

      bool _validSection = true;
      if (sectionJson.containsKey("channel") && sectionJson["channel"].is<uint8_t>())
        section.channel = sectionJson["channel"].as<uint8_t>();
      else
        _validSection = false;
      
      if (sectionJson.containsKey("start") && sectionJson["start"].is<uint16_t>())
        section.start = sectionJson["start"].as<uint16_t>();
      else
        _validSection = false;

      if (sectionJson.containsKey("end") && sectionJson["end"].is<uint16_t>())
        section.end = sectionJson["end"].as<uint16_t>();
      else
        _validSection = false;

      if (_validSection) {
        sections.push_back(section);
        uint16_t count = section.end - section.start;
        region.count += count;
        region.breaks.push_back(count);
      }
    }
    region.sections = sections;
    regions[regionName] = region;
  }

  // set lights config
  config.channels = channels;
  config.regions = regions;

  ampConfig.lights = config;
}

void Config::loadActionConfig(JsonObject actionJson) {
  std::map<std::string, std::vector<LightingParameters>*> actions;

  for (auto actionPair : actionJson) {
    std::string action = std::string(actionPair.key().c_str());
    actions[action] = new std::vector<LightingParameters>();
    JsonArray regionEffects = actionPair.value().as<JsonArray>();
    
    // parse all regional effects for action
    for (auto regionEffect : regionEffects) {
      if (regionEffect.containsKey("region") && regionEffect.containsKey("effect")) {
        LightingParameters effect;

        // validate and add lighting effect
        if (parseEffect(regionEffect["effect"], &effect)) {
          effect.region = regionEffect["region"].as<std::string>();
          actions[action]->push_back(effect);
        }
      }
    }
  }

  ampConfig.actions = actions;
}

std::string Config::readFile(std::string filename) {
  if (!_filesystemError)
    return ampStorage.readFile(filename);
  
  ESP_LOGE(CONFIG_TAG,"Unable to read file due to filesystem error");
  return "";
}

DeviceInfo Config::getDeviceInfo() {
  DeviceInfo info;
  info.hardwareVersion = AmpStorage::getHardwareRevision();
  info.serialNumber = AmpStorage::getSerialNumber();
  info.deviceName = AmpStorage::getDeviceName();

  return info;
}

bool Config::parseEffect(std::string data, LightingParameters *params) {
  auto parts = split(data, ',');
  params->effect = (LightEffect) atoi(parts[0].c_str());
  auto numParts = parts.size();

  switch (params->effect) {
    case LightEffect::Static:
      if (numParts < 2) {
        ESP_LOGW(CONFIG_TAG, "Missing required number of args for light effect %d", params->effect);
        return false;
      }

      params->first = hexToColor(parts[1]);

      if (numParts == 3)
        params->layer = atoi(parts[2].c_str());
      break;
    case LightEffect::Scan:
    case LightEffect::ColorWipe:
    case LightEffect::Blink:
    case LightEffect::Breathe:
    case LightEffect::Fade:
    case LightEffect::Twinkle:
    case LightEffect::Sparkle:
    case LightEffect::Alternate:
    case LightEffect::TheaterChase:
      if (numParts < 4) {
        ESP_LOGW(CONFIG_TAG, "Missing required number of args for light effect %d", params->effect);
        return false;
      }

      params->first = hexToColor(parts[1]);
      params->second = hexToColor(parts[2]);
      params->duration = atoll(parts[3].c_str());

      if (numParts == 5)
        params->layer = atoi(parts[4].c_str());
      break;
    case LightEffect::Rainbow:
    case LightEffect::RainbowCycle:
    case LightEffect::TheaterChaseRainbow:
      if (numParts < 2) {
        ESP_LOGW(CONFIG_TAG, "Missing required number of args for light effect %d", params->effect);
        return false;
      }

      params->duration = atoll(parts[1].c_str());

      if (numParts == 3)
        params->layer = atoi(parts[2].c_str());
      break;
    case LightEffect::Off:
      params->first = lightOff;
    default:
      break;
  }

  return true;
}