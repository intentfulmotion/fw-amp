#include <hal/lights.h>

Lights::Lights() {
  touchQueue = xQueueCreate(2, sizeof(bool));
  calibrateXGQueue = xQueueCreate(1, sizeof(CalibrationState));
  calibrateMagQueue = xQueueCreate(1, sizeof(CalibrationState));
  configUpdatedQueue = xQueueCreate(1, sizeof(bool));
  powerStatusQueue = xQueueCreate(1, sizeof(PowerStatus));
  updateStatusQueue = xQueueCreate(5, sizeof(UpdateStatus));
  advertisingQueue = xQueueCreate(1, sizeof(bool));
}

void Lights::onPowerUp() {
  leds.init();
  ESP_LOGD(LIGHTS_TAG,"Lights started");
}

void Lights::onPowerDown() {
  leds.deinit();
  ESP_LOGD(LIGHTS_TAG,"Lights stopped");
}

void Lights::process() {
  if (uxQueueMessagesWaiting(touchQueue)) {
    bool touched;
    if (xQueueReceive(touchQueue, &touched, 0))
      touched ? onTouchDown() : onTouchUp();
  }

  if (uxQueueMessagesWaiting(calibrateXGQueue)) {
    CalibrationState state;
    if (xQueueReceive(calibrateXGQueue, &state, 0))
      state == CalibrationState::Started ? onCalibrateXGStarted() : onCalibrateXGEnded();
  }
  else if (uxQueueMessagesWaiting(calibrateMagQueue)) {
    CalibrationState state;
    if (xQueueReceive(calibrateMagQueue, &state, 0))
      state == CalibrationState::Started ? onCalibrateMagStarted() : onCalibrateMagEnded();
  }

  if (uxQueueMessagesWaiting(configUpdatedQueue)) {
    bool valid;
    if (xQueueReceive(configUpdatedQueue, &valid, 0) && valid)
      onConfigUpdated();
  }

  if (uxQueueMessagesWaiting(powerStatusQueue)) {
    PowerStatus status;
    if (xQueueReceive(powerStatusQueue, &status, 0))
      onPowerStatusChanged(status);
  }

  if (uxQueueMessagesWaiting(updateStatusQueue)) {
    UpdateStatus status;
    if (xQueueReceive(updateStatusQueue, &status, 0))
      onUpdateStatusChanged(status);
  }

  if (uxQueueMessagesWaiting(advertisingQueue)) {
    uint8_t status;
    if (xQueueReceive(advertisingQueue, &status, 0))
      status == 1 ? onAdvertisingStarted() : onAdvertisingStopped();
  }

  leds.process();
}

void Lights::onAdvertisingStarted() {
  advertising = true;
  xTaskCreate(startAdvertisingLight, "advertise-light", 2048, this, 3, &advertisingLightHandle);
}

void Lights::onAdvertisingStopped() {
  advertising = false;
  vTaskDelete(advertisingLightHandle);
  advertisingLightHandle = NULL;
}

void Lights::onUpdateStatusChanged(UpdateStatus status) {
  ESP_LOGD(LIGHTS_TAG,"Updating light for update status change");
  if (_updateStatus != status) {
    _updateStatus = status;

    if (updateLightHandle != NULL) {
      vTaskDelete(updateLightHandle);
      updateLightHandle = NULL;
      updateLightForPowerStatus(_powerStatus);
    }

    switch (_updateStatus) {
      case UpdateStatus::Start:
        updating = true;
        setStatus(updateStart);
        break;
      case UpdateStatus::End:
        updating = false;
        setStatus(updateEnd);
      break;
      case UpdateStatus::Write:
        setStatus(updateToggle);
        xTaskCreate(startUpdateLight, "update-light", 2048, this, 3, &updateLightHandle);
        break;
      default:
        updating = false;
      break;
    }

    leds.render(false);
  }
}

void Lights::onPowerStatusChanged(PowerStatus status) {
  _powerStatus = status;
  ESP_LOGD(LIGHTS_TAG,"Updating light for power status change");
  updateLightForPowerStatus(status);
}

void Lights::updateLightForPowerStatus(PowerStatus status) {
  if (!updating && !advertising) {
    if (status.charging) {
      if (status.level != PowerLevel::Charged) {
        leds.setStatus(ampOrange);
        ESP_LOGV(LIGHTS_TAG,"Setting to orange charging light");
      }
      else {
        leds.setStatus(Color(0, 127, 0));
        ESP_LOGV(LIGHTS_TAG,"Setting to green charged light");
      }
    }
    else {
      switch (status.level) {
        case PowerLevel::Critical:
          leds.setStatus(ColorRGB(127, 0, 0));
          leds.setBrightness(60);
          safeToLight = false;
          ESP_LOGV(LIGHTS_TAG,"Setting to red critical light");
          break;
        case PowerLevel::Low:
          leds.setStatus(ColorRGB(127, 127, 0));
          leds.setBrightness(127);
          safeToLight = true;
          ESP_LOGV(LIGHTS_TAG,"Setting to yellow low light");
          break;
        case PowerLevel::Charged:
        case PowerLevel::Unknown:
        case PowerLevel::Normal:
        default:
          leds.setStatus(ampPink);
          leds.setBrightness(255);
          safeToLight = true;
          ESP_LOGV(LIGHTS_TAG,"Setting to amp pink normal light");
          break;
      }

      leds.render(true);
    }
  }
}

void Lights::setStatus(ColorRGB color) {
  leds.setStatus(color);
}

void Lights::onTouchDown() {
  leds.setStatus(ampPurple);
}

void Lights::onTouchUp() {
  updateLightForPowerStatus(_powerStatus);
}

void Lights::onConfigUpdated() {
  lightsConfig = &Config::ampConfig.lights;
  
  for (auto channel : lightsConfig->channels) {
    auto channelNum = channel.second.channel;

    // don't allow channels 5 - 8 to be added if the corresponding 1 - 4 channel is a DotStar
    if (channelNum < 5 || (channelNum >= 5 && lightsConfig->channels[channelNum - 4].type != 2))
      controllers[channel.first] = leds.addLEDStrip(channel.second);
  }

  init = true;
}

LightRegion Lights::getLightRegion(std::string name) {
  return lightsConfig->regions[name];
}

std::map<uint8_t, LightChannel> Lights::getAvailableChannels() {
  return lightsConfig->channels;
}

std::map<std::string, LightRegion> Lights::getAvailableRegions() {
  return lightsConfig->regions;
}

void Lights::colorRegion(std::string region, ColorRGB color, ColorRGBW colorExtended) {
  auto result = lightsConfig->regions.find(region);

  if (result != lightsConfig->regions.end()) {
    ESP_LOGV(LIGHTS_TAG,"Found region: %s", region.c_str());

    auto region = result->second;
    for (auto section : region.sections) {
      ESP_LOGV(LIGHTS_TAG,"Color section: %d (%d - %d) -> RGB(%d, %d, %d)", section.channel, section.start, section.end, color.r, color.g, color.b);
      colorLEDs(section.channel, section.start, section.end, color);
    }
  }
}

void Lights::colorRegionSection(std::string region, uint8_t sectionIndex, ColorRGB color, ColorRGBW colorExtended) {
  auto result = lightsConfig->regions.find(region);

  if (result != lightsConfig->regions.end()) {
    auto region = result->second;
    if (sectionIndex < region.sections.size()) {
      auto section = region.sections[sectionIndex];
      colorLEDs(section.channel, section.start, section.end, color, colorExtended);
    }
  }
}

void Lights::colorLEDs(uint8_t channel, uint16_t start, uint16_t end, Color color) {
  ESP_LOGV(LIGHTS_TAG,"Color section: %d (%d - %d)", channel, start, end);
  auto corrected = leds.gammaCorrected(color);
  leds.setPixels(channel, corrected, start, end);
}

void Lights::render(bool all, int8_t channel) {
  ESP_LOGV(LIGHTS_TAG,"Rendering lights");
  leds.render(all, channel);
}

void Lights::onCalibrateXGStarted() {
  calibratingXG = true;
  calibratingMag = false;

  xTaskCreate(startCalibrateLight, "calibrate-light", 2048, this, 3, &calibrateLightHandle);
}

void Lights::onCalibrateXGEnded() {
  calibratingXG = false;

  if (calibrateLightHandle != NULL) {
    vTaskDelete(calibrateLightHandle);
    calibrateLightHandle = NULL;
  }
}

void Lights::onCalibrateMagStarted() {
  calibratingXG = false;
  calibratingMag = true;

  xTaskCreate(startCalibrateLight, "calibrate-light", 2048, this, 3, &calibrateLightHandle);
}

void Lights::onCalibrateMagEnded() {
  calibratingMag = false;

  if (calibrateLightHandle != NULL) {
    vTaskDelete(calibrateLightHandle);
    calibrateLightHandle = NULL;
  }
}

void Lights::startCalibrateLight(void* params) {
  Lights* lights = (Lights*)params;
  auto color = lights->calibratingXG ? ColorRGB(127, 127, 0) : ColorRGB(0, 127, 127);

  for (;;) {
    lights->setStatus(color);
    delay(200);
    lights->setStatus(lightOff);
    delay(200);
  }

  vTaskDelete(NULL);
}

void Lights::startUpdateLight(void* params) {
  Lights* lights = (Lights*)params;
  auto color = ampOrange;

  for (;;) {
    lights->setStatus(color);
    delay(200);
    lights->setStatus(lightOff);
    delay(200);
  }

  vTaskDelete(NULL);
}

void Lights::startAdvertisingLight(void* params) {
  Lights* lights = (Lights*)params;
  auto color = ColorRGB(0, 0, 127);

  for (;;) {
    lights->setStatus(color);
    delay(100);
    lights->setStatus(lightOff);
    delay(100);
  }

  vTaskDelete(NULL);
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
ColorRGB Lights::Wheel(uint8_t WheelPos) {
  if(WheelPos < 85)
    return ColorRGB(WheelPos * 3, 255 - WheelPos * 3, 0);
  else if(WheelPos < 170) {
    WheelPos -= 85;
    return ColorRGB(255 - WheelPos * 3, 0, WheelPos * 3);
  } 
  else {
    WheelPos -= 170;
    return ColorRGB(0, WheelPos * 3, 255 - WheelPos * 3);
  }
}

ColorRGB Lights::randomColorRGB() {
  return Wheel(rand() % 256); 
}

