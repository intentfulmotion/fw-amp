#include <hal/config.h>
#include <hal/lights.h>

AmpConfig Config::ampConfig;

FreeRTOS::Semaphore Config::effectsUpdating = FreeRTOS::Semaphore("effects");

void Config::onPowerUp()
{
  if (!ampStorage.init())
  {
    ESP_LOGE(CONFIG_TAG, "Unable to mount filsystem");
    _filesystemError = true;
  }

  ampConfig.info = getDeviceInfo();

  ESP_LOGI(CONFIG_TAG, "Amp %s", ampConfig.info.hardwareVersion.c_str());
  ESP_LOGI(CONFIG_TAG, "Firmware: %s", ampConfig.info.firmwareVersion.c_str());
  ESP_LOGI(CONFIG_TAG, "Serial Number: %s", ampConfig.info.serialNumber.c_str());
  ESP_LOGI(CONFIG_TAG, "Copyright %d %s", COPYRIGHT_YEAR, ampConfig.info.manufacturer.c_str());
  ESP_LOGI(CONFIG_TAG, "IDF version: %s", esp_get_idf_version());

  if (ampStorage.fileExists(userConfigPath) && loadConfigFile(userConfigPath))
  {
    _isUserConfig = true;
    _valid = true;
    loadConfig();

    notifyConfigListeners();
  }
  else if (ampStorage.fileExists(configPath) && loadConfigFile(configPath))
  {
    _valid = true;
    loadConfig();

    notifyConfigListeners();
  }
  else
    ESP_LOGW(CONFIG_TAG, "No valid configuration exists on this Amp");
}

void Config::onPowerDown()
{
  ampStorage.deinit();
}

void Config::process() {}

void Config::addConfigListener(ConfigListener *listener)
{
  configListeners.push_back(listener);
}

void Config::notifyConfigListeners()
{
  for (auto listener : configListeners)
    if (listener->configUpdatedQueue != NULL)
      xQueueSendToFront(listener->configUpdatedQueue, &_valid, 0);
}

bool Config::loadConfigFile(std::string path)
{
  auto file = ampStorage.openFile(path);

  if (!_filesystemError)
  {
    MsgPackFileReader reader(file);
    auto err = deserializeMsgPack(document, reader);
    if (err != DeserializationError::Ok)
      ESP_LOGE(CONFIG_TAG, "Deserialization error: %s", err.c_str());
    return err == DeserializationError::Ok;
  }
  else
  {
    ESP_LOGE(CONFIG_TAG, "Cannot load config due to error with filesystem.");
    return false;
  }
}

void Config::saveConfig()
{
  std::string path = _isUserConfig ? userConfigPath : configPath;
  auto file = ampStorage.writeFile(path);

  if (!file)
  {
    ESP_LOGE(CONFIG_TAG, "Could not open file: %s", path.c_str());
    return;
  }

  // serialize effects
  ESP_LOGD(CONFIG_TAG, "Writing config to file");
  MsgPackFileWriter writer(file);
  serializeMsgPack(document, writer);

  fclose(file);
}

void Config::saveUserConfig(std::string data, bool load)
{
  auto file = ampStorage.writeFile(userConfigPath);

  if (!file)
  {
    ESP_LOGE(CONFIG_TAG, "Could not open file: %s", userConfigPath.c_str());
    return;
  }

  ESP_LOGD(CONFIG_TAG, "Writing user config to file");
  fputs(data.c_str(), file);
  fclose(file);

  if (load)
  {
    auto err = deserializeMsgPack(document, data);
    if (err == DeserializationError::Ok)
    {
      _isUserConfig = true;
      loadConfig();
      notifyConfigListeners();
    }
  }
}

void Config::loadConfig()
{
  JsonObject configJson = document.as<JsonObject>();

  JsonObject lightsJson = configJson["lights"].as<JsonObject>();
  loadLightsConfig(lightsJson);

  JsonObject actionsJson = configJson["actions"].as<JsonObject>();
  loadActionConfig(actionsJson);

  JsonObject motionJson = configJson["motion"].as<JsonObject>();
  loadMotionConfig(motionJson);

  JsonObject batteryJson = configJson["battery"].as<JsonObject>();
  loadBatteryConfig(batteryJson);

  JsonObject balanceJson = configJson["balance"].as<JsonObject>();
  loadBalanceConfig(balanceJson);

  ESP_LOGD(CONFIG_TAG, "Loaded configuration");
}

void Config::loadMotionConfig(JsonObject motionJson)
{
  MotionConfig config;

  config.autoOrientation = motionJson["autoOrientation"] | false;
  config.autoMotion = motionJson["autoMotion"] | false;
  config.autoTurn = motionJson["autoTurn"] | false;
  config.autoDirection = motionJson["autoDirection"] | false;
  config.relativeTurnZero = motionJson["relativeTurnZero"] | true;

  config.brakeThreshold = motionJson["brakeThreshold"] | DEFAULT_BRAKE_THRESHOLD;
  config.accelerationThreshold = motionJson["accelerationThreshold"] | DEFAULT_ACCELERATION_THRESHOLD;
  config.turnThreshold = motionJson["turnThreshold"] | DEFAULT_TURN_THRESHOLD;

  uint8_t motionAxis = (motionJson["motionAxis"].as<uint8_t>()) | AccelerationAxis::X_Pos;
  config.motionAxis = (AccelerationAxis)motionAxis;

  uint8_t turnAxis = (AttitudeAxis)(motionJson["turnAxis"].as<uint8_t>()) | AttitudeAxis::Roll;
  config.turnAxis = (AttitudeAxis)turnAxis;

  uint8_t orientationTrigger = (Orientation)(motionJson["orientation"].as<uint8_t>()) | Orientation::UnknownSideUp;
  config.orientationTrigger = (Orientation)orientationTrigger;

  uint8_t directionTrigger = (DirectionTrigger)(motionJson["directionTrigger"].as<uint8_t>()) | DirectionTrigger::Attitude;
  config.directionTriggerType = (DirectionTrigger)directionTrigger;

  uint8_t directionAxis = motionJson["directionAxis"].as<uint8_t>();
  config.directionThreshold = motionJson["directionThreshold"];

  switch (config.directionTriggerType)
  {
  case DirectionTrigger::Acceleration:
    config.directionAccelerationAxis = (AccelerationAxis)directionAxis;
    break;

  case DirectionTrigger::Attitude:
    config.directionAttitudeAxis = (AttitudeAxis)directionAxis;
  default:
    break;
  }ESP_LOGD(CAN_TAG, "Direction changed");

  ESP_LOGW(CONFIG_TAG, "auto orientation config: %s", config.autoOrientation ? "true" : "false");
  ESP_LOGW(CONFIG_TAG, "auto motion config: %s", config.autoMotion ? "true" : "false");
  ESP_LOGW(CONFIG_TAG, "auto turn config: %s", config.autoTurn ? "true" : "false");
  ESP_LOGW(CONFIG_TAG, "auto direction config: %s", config.autoDirection ? "true" : "false");

  ESP_LOGW(CONFIG_TAG, "motion axis: %d brake threshold: %.2f acceleration threshold: %.2f", config.motionAxis, config.brakeThreshold, config.accelerationThreshold);
  ESP_LOGW(CONFIG_TAG, "orientation trigger: %d", config.orientationTrigger);
  ESP_LOGW(CONFIG_TAG, "direction type: %d, axis: %d, threshold: %.2f", config.directionTriggerType, directionAxis, config.directionThreshold);

  ampConfig.motion = config;
}

void Config::loadLightsConfig(JsonObject lightsJson)
{
  LightsConfig config;

  // load light channels
  JsonArray channelsJson = lightsJson["channels"].as<JsonArray>();
  std::map<uint8_t, LightChannel> channels;
  for (auto channel : channelsJson)
  {
    LightChannel ch;
    ch.channel = channel["channel"].as<uint8_t>();
    ch.leds = channel["leds"].as<uint16_t>();
    ch.type = channel["type"].as<LEDType>();
    channels[ch.channel] = ch;
  }

  // load light regions
  std::map<std::string, LightRegion> regions;
  for (auto lightRegion : lightsJson["regions"].as<JsonObject>())
  {
    std::string regionName = std::string(lightRegion.key().c_str());
    JsonArray sectionsJson = lightRegion.value().as<JsonArray>();

    LightRegion region;
    std::vector<LightSection> sections;
    region.count = 0;

    for (auto sect : sectionsJson)
    {
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

      if (_validSection)
      {
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

void Config::loadActionConfig(JsonObject actionJson)
{
  for (auto actionPair : actionJson)
  {
    std::string action = std::string(actionPair.key().c_str());
    JsonArray regionEffects = actionPair.value().as<JsonArray>();

    // parse all regional effects for action
    for (auto regionEffect : regionEffects)
    {
      if (regionEffect.containsKey("region") && regionEffect.containsKey("effect"))
      {
        std::string region = regionEffect["region"].as<std::string>();
        std::string effect = regionEffect["effect"].as<std::string>();
        if (!addEffect(action, region, effect))
          ESP_LOGW(CONFIG_TAG, "Unable to add effect - action: %s\tregion: %s\teffect: %s",
                   action.c_str(), region.c_str(), effect.c_str());
        else
          ESP_LOGD(CONFIG_TAG, "Added effect - action: %s\tregion: %s\teffect: %s",
                   action.c_str(), region.c_str(), effect.c_str());
      }
    }
  }
}

void Config::loadBatteryConfig(JsonObject batteryJson)
{
  BatteryConfig config;
  if (batteryJson.containsKey("voltageBreakpoints"))
  {
    auto breakpoints = batteryJson["voltageBreakpoints"].as<JsonArray>();
    for (int i = 0; i < VOLTAGE_BREAKPOINTS; i++)
    {
      config.voltageBreakpoints[i] = breakpoints.getElement(i).as<float>();
    }
  }
  else
    for (int i = 0; i < VOLTAGE_BREAKPOINTS; i++)
    {
      config.voltageBreakpoints[i] = DEFAULT_VOLTAGE_BREAKPOINTS[i];
    }

  ampConfig.battery = config;
}

void Config::loadBalanceConfig(JsonObject balanceJson)
{
  BalanceConfig config;
  if (balanceJson.containsKey("tiltback"))
  {
    BalanceTiltbackConfig tiltbackConfig;
    auto tiltbackJson = balanceJson["tiltback"].as<JsonObject>();
    tiltbackConfig.highVoltage = tiltbackJson["highVoltage"];
    tiltbackConfig.lowVoltage = tiltbackJson["lowVoltage"];
    tiltbackConfig.dutyCycle = tiltbackJson["dutyCycle"];
    config.tiltback = tiltbackConfig;

    ESP_LOGD(CONFIG_TAG, "Tiltback config - voltage range >= %.2f and <= %.2f w/ duty cycle %.2f", config.tiltback.lowVoltage, config.tiltback.highVoltage, config.tiltback.dutyCycle);
  }

  if (balanceJson.containsKey("footpads"))
  {
    BalanceFootpadsConfig footpadsConfig;
    auto footpadsJson = balanceJson["footpads"].as<JsonObject>();
    footpadsConfig.adc1 = footpadsJson["adc1"];
    footpadsConfig.adc2 = footpadsJson["adc2"];
    config.footpads = footpadsConfig;

    ESP_LOGD(CONFIG_TAG, "Footpad config - adc1: %.2f, adc2: %.2f", config.footpads.adc1, config.footpads.adc2);
  }

  ampConfig.balance = config;
}

std::string Config::readFile(std::string filename)
{
  if (!_filesystemError)
    return ampStorage.readFile(filename);

  ESP_LOGE(CONFIG_TAG, "Unable to read file due to filesystem error");
  return "";
}

DeviceInfo Config::getDeviceInfo()
{
  DeviceInfo info;
  info.hardwareVersion = AmpStorage::getHardwareRevision();
  info.serialNumber = AmpStorage::getSerialNumber();
  info.deviceName = AmpStorage::getDeviceName();

  return info;
}

bool Config::addEffect(std::string action, std::string region, std::string data, bool updateJson)
{
  effectsUpdating.wait(CONFIG_TAG);
  effectsUpdating.take(CONFIG_TAG);

  bool found = false;

  auto hasForwardSuffix = hasEnding(action, "-forward");
  auto hasBackwardSuffix = hasEnding(action, "-backward");

  std::string normalized = action;
  if (hasForwardSuffix)
    normalized.erase(normalized.length() - forwardSuffixLength, forwardSuffixLength);
  if (hasBackwardSuffix)
    normalized.erase(normalized.length() - backwardSuffixLength, backwardSuffixLength);

  for (auto const &[command, actionName] : Lights::headlightActions)
    if (actionName.compare(normalized) == 0)
    {
      found = true;
      break;
    }

  if (!found)
    for (auto const &[command, actionName] : Lights::turnActions)
      if (actionName.compare(normalized) == 0)
      {
        found = true;
        break;
      }

  if (!found)
    for (auto const &[command, actionName] : Lights::motionActions)
      if (actionName.compare(normalized) == 0)
      {
        found = true;
        break;
      }

  if (!found)
    for (auto const &[command, actionName] : Lights::faultActions)
      if (actionName.compare(normalized) == 0)
      {
        found = true;
        break;
      }

  if (!found)
    for (auto const &[command, actionName] : Lights::batteryActions)
      if (actionName.compare(normalized) == 0)
      {
        found = true;
        break;
      }

  if (!found)
    return false;

  LightingParameters effect;
  if (!parseEffect(data, &effect))
  {
    ESP_LOGW(CONFIG_TAG, "Unable to parse effect %s for region %s", data.c_str(), region.c_str());
    return false;
  }

  effect.region = region;

  if (ampConfig.actions.find(action) == ampConfig.actions.end())
    ampConfig.actions[action] = new std::vector<LightingParameters>();

  ampConfig.actions[action]->push_back(effect);

  if (updateJson)
  {
    auto actionsRoot = document["actions"].as<JsonObject>();
    JsonArray actionRoot;

    if (!actionsRoot.containsKey(action))
      actionRoot = actionsRoot.createNestedArray(action);
    else
      actionRoot = actionsRoot[action].as<JsonArray>();

    bool foundJson = false;
    for (auto effect : actionRoot)
    {
      if (effect[region].as<std::string>().compare(region) == 0)
      {
        foundJson = true;
        effect["effect"] = data.c_str();
      }
    }
    if (!foundJson)
    {
      auto effectRoot = actionRoot.createNestedObject();
      effectRoot["region"] = region.c_str();
      effectRoot["effect"] = data.c_str();
    }
  }

  effectsUpdating.give();

  return true;
}

std::vector<LightingParameters> *Config::getActionEffects(std::string action)
{
  if (ampConfig.actions.find(action) == ampConfig.actions.end())
    return NULL;

  return ampConfig.actions[action];
}

// void Config::removeEffect(std::string action, std::string region, bool updateJson) {
//   effectsUpdating.wait(CONFIG_TAG);
//   effectsUpdating.take(CONFIG_TAG);

//   if (ampConfig.actions.find(action) == ampConfig.actions.end())
//     return;

//   for (auto it = ampConfig.actions[action]->begin(); it != ampConfig.actions[action]->end(); ++it) {
//     auto effect = *it;
//     if (effect.region.compare(region) == 0)
//       ampConfig.actions[action]->erase(it);
//   }

//   if (updateJson) {
//     auto actionsRoot = document["actions"].as<JsonObject>();
//     if (!actionsRoot.containsKey(action))
//       return;

//     auto actionRoot = actionsRoot[action];
//     for (int i = 0; i < actionRoot.size(); i++) {
//       auto effect = actionRoot[i];
//       if (strcmp(effect["region"], region.c_str()) == 0) {
//         ESP_LOGD(CONFIG_TAG, "Found effect - removing %s for %s", action.c_str(), region.c_str());
//         actionRoot.remove(i);
//         document.garbageCollect();
//         break;
//       }
//     }
//   }

//   effectsUpdating.give();
// }

bool Config::parseEffect(std::string data, LightingParameters *params)
{
  auto parts = split(data, ',');
  params->effect = (LightEffect)atoi(parts[0].c_str());
  params->layer = 0;
  auto numParts = parts.size();

  switch (params->effect)
  {
  case LightEffect::Static:
    if (numParts < 2)
    {
      ESP_LOGW(CONFIG_TAG, "Missing required number of args for light effect %d", params->effect);
      return false;
    }

    params->first = parseColorOption(parts[1]);

    if (numParts == 3)
      params->layer = atoi(parts[2].c_str());
    break;
  case LightEffect::TheaterChase:
    if (numParts < 3)
    {
      ESP_LOGW(CONFIG_TAG, "Missing required number of args for light effect %d", params->effect);
      return false;
    }

    params->first = parseColorOption(parts[1]);
    params->duration = atoll(parts[2].c_str());

    if (numParts == 4)
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
  case LightEffect::ColorChase:
    if (numParts < 4)
    {
      ESP_LOGW(CONFIG_TAG, "Missing required number of args for light effect %d", params->effect);
      return false;
    }

    params->first = parseColorOption(parts[1]);
    params->second = parseColorOption(parts[2]);
    params->duration = atoll(parts[3].c_str());

    if (numParts == 5)
      params->layer = atoi(parts[4].c_str());
    break;
  case LightEffect::Rainbow:
  case LightEffect::RainbowCycle:
    if (numParts < 2)
    {
      ESP_LOGW(CONFIG_TAG, "Missing required number of args for light effect %d", params->effect);
      return false;
    }

    params->duration = atoll(parts[1].c_str());

    if (numParts == 3)
      params->layer = atoi(parts[2].c_str());
    break;
  case LightEffect::Battery:
    if (numParts >= 2)
      params->first = parseColorOption(parts[1]);
    else
      params->first = ColorOption{ampPink, false, false};

    if (numParts >= 3)
      params->layer = atoi(parts[2].c_str());
    break;
  case LightEffect::Transparent:
  case LightEffect::Off:
    params->first = {lightOff, false, false};

    if (numParts == 2)
      params->layer = atoi(parts[1].c_str());
  default:
    break;
  }

  return true;
}

ColorOption Config::parseColorOption(std::string data)
{
  ColorOption option = {lightOff, false, false};

  if (data == "random")
    option.random = true;
  else if (data == "rainbow")
    option.rainbow = true;
  else
    option.color = hexToColor(data);

  return option;
}