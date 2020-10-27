#include <hal/lights.h>

std::map<Actions, std::string> Lights::headlightActions = {
  std::make_pair(Actions::LightsOff, "headlight-off"),
  std::make_pair(Actions::LightsHeadlightNormal, "headlight-normal"),
  std::make_pair(Actions::LightsHeadlightBright, "headlight-bright")
};

std::map<Actions, std::string> Lights::motionActions = {
  std::make_pair(Actions::LightsOff, "motion-off"),
  std::make_pair(Actions::LightsMotionNeutral, "motion-neutral"),
  std::make_pair(Actions::LightsMotionBrakes, "motion-brakes"),
  std::make_pair(Actions::LightsMotionAcceleration, "motion-acceleration")
};

std::map<Actions, std::string> Lights::turnActions = {
  std::make_pair(Actions::LightsOff, "turn-off"),
  std::make_pair(Actions::LightsTurnCenter, "turn-center"),
  std::make_pair(Actions::LightsTurnLeft, "turn-left"),
  std::make_pair(Actions::LightsTurnRight, "turn-right"),
  std::make_pair(Actions::LightsTurnHazard, "turn-hazard")
};

std::map<Actions, std::string> Lights::orientationActions = {
  std::make_pair(Actions::LightsOff, "orientation-off"),
  std::make_pair(Actions::LightsOrientationUnknown, "orientation-unknown"),
  std::make_pair(Actions::LightsOrientationTop, "orientation-top"),
  std::make_pair(Actions::LightsOrientationBottom, "orientation-bottom"),
  std::make_pair(Actions::LightsOrientationLeft, "orientation-left"),
  std::make_pair(Actions::LightsOrientationRight, "orientation-right"),
  std::make_pair(Actions::LightsOrientationFront, "orientation-front"),
  std::make_pair(Actions::LightsOrientationBack, "orientation-back")
};

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
  xTaskCreatePinnedToCore(renderer, "renderer", 4096, NULL, 3, &renderHandle, 1);
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
          leds.setStatus(Color(127, 0, 0));
          leds.setBrightness(60);
          safeToLight = false;
          ESP_LOGV(LIGHTS_TAG,"Setting to red critical light");
          break;
        case PowerLevel::Low:
          leds.setStatus(Color(127, 127, 0));
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

void Lights::setStatus(Color color) {
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

void Lights::colorRegion(std::string regionName, Color color) {
  auto region = lightsConfig->regions[regionName];

  for (auto section : region.sections) {
    ESP_LOGV(LIGHTS_TAG,"Color section: %d (%d - %d) -> RGB(%d, %d, %d)", section.channel, section.start, section.end, color.r, color.g, color.b);
    colorLEDs(section.channel, section.start, section.end, color);
  }
}

void Lights::colorRegionSection(std::string regionName, uint8_t sectionIndex, Color color) {
  auto region = lightsConfig->regions[regionName];

  if (sectionIndex < region.sections.size()) {
    auto section = region.sections[sectionIndex];
    colorLEDs(section.channel, section.start, section.end, color);
  }
}

void Lights::colorLEDs(uint8_t channel, uint16_t start, uint16_t end, Color color) {
  ESP_LOGV(LIGHTS_TAG,"Color section: %d (%d - %d)", channel, start, end);
  auto corrected = leds.gammaCorrected(color);
  leds.setPixels(channel, corrected, start - 1, end);
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
  auto color = lights->calibratingXG ? Color(127, 127, 0) : Color(0, 127, 127);

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
  auto color = Color(0, 0, 127);

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
Color Lights::colorWheel(uint8_t pos) {
  if(pos < 85)
    return Color(pos * 3, 255 - pos * 3, 0);
  else if(pos < 170) {
    pos -= 85;
    return Color(255 - pos * 3, 0, pos * 3);
  } 
  else {
    pos -= 170;
    return Color(0, pos * 3, 255 - pos * 3);
  }
}

Color Lights::randomColor() {
  return colorWheel(rand() % 256); 
}

void Lights::applyEffect(LightingParameters parameters) {
  auto region = parameters.region;

  // replace existing effect if it exists + reset render steps
  if (lightsConfig->regions.find(region) != lightsConfig->regions.end()) {
    LightingParameters last;
    bool replacingOldEffect = false;

    // see if we've got an existing effect for this region
    if (_effects.find(region) != _effects.end()) {
      last = _effects[region];
      replacingOldEffect = true;
    }

    // set the new effect
    _effects[region] = parameters;

    // do teardown of step data for the old effect
    if (replacingOldEffect)
      endEffect(last);

    // initialize step data for effect
    startEffect(parameters);
  }
  else
    ESP_LOGW(LIGHTS_TAG, "Cannot apply effect - Region %s does not exist.", region.c_str());
}

void Lights::startEffect(LightingParameters parameters) {
  switch (parameters.effect) {
    case LightEffect::Sparkle:
      _steps[parameters.region] = { 0, millis(), new uint32_t(0) };
      break;
    case LightEffect::Scan:
      _steps[parameters.region] = { 0, millis(), new bool(1) };
      break;
    default:
      _steps[parameters.region] = { 0, millis(), NULL };
  }
}

void Lights::endEffect(LightingParameters parameters) {
  switch (parameters.effect) {
    case LightEffect::Sparkle:
      delete (uint32_t*) _steps[parameters.region].data;
      break;
    case LightEffect::Scan:
      delete (bool*) _steps[parameters.region].data;
      break;
    default:
      break;
  }
}

void Lights::renderer(void *args) {
  auto lights = Lights::instance();
  std::priority_queue<LightingParameters, std::vector<LightingParameters>, std::greater<LightingParameters>> compositor;
  std::vector<LightingParameters> staticEffects;

  for (;;) {
    // process any messages
    lights->process();

    // schedule effects to be rendered
    auto now = millis();
    for (auto const& [region, step] : lights->_steps) {
      auto& effect = lights->_effects[region];

      if (step.next != REFRESH_NEVER && step.next <= now)
        compositor.push(effect);
      else if (effect.effect == LightEffect::Static || effect.effect == LightEffect::Off)
        staticEffects.push_back(effect);
    }

    bool updatesNeeded = compositor.size() > 0;
    
    // re-render static/off effects if we've got updates
    if (updatesNeeded)
      for (auto& effect : staticEffects)
        compositor.push(effect);

    staticEffects.clear();

    // apply effects
    while (compositor.size() > 0) {
      auto next = compositor.top();
      auto& effect = lights->_effects[next.region];
      auto& step = lights->_steps[next.region];
      ESP_LOGV(LIGHTS_TAG, "Painting effect %d on %s", effect.effect, next.region.c_str());
      lights->renderLightingEffect(&effect, &step);
      compositor.pop();
    }

    // render lights
    if (updatesNeeded)
      lights->render(true);
      
    delay(10);
  }
}

void Lights::renderLightingEffect(LightingParameters *params, RenderStep *step) {
  switch (params->effect) {
    case LightEffect::Off:
    case LightEffect::Static:
      color(params, step);
      break;
    case LightEffect::Blink:
      blink(params, step);
      break;
    case LightEffect::Alternate:
      alternate(params, step);
      break;
    case LightEffect::ColorWipe:
      colorWipe(params, step);
      break;
    case LightEffect::Breathe:
      breathe(params, step);
      break;
    case LightEffect::Fade:
      fade(params, step);
      break;
    case LightEffect::Scan:
      scan(params, step);
      break;
    case LightEffect::Rainbow:
      rainbow(params, step);
      break;
    case LightEffect::RainbowCycle:
      rainbowCycle(params, step);
      break;
    case LightEffect::ColorChase:
      colorChase(params, step);
      break;
    case LightEffect::TheaterChase:
      theaterChase(params, step);
      break;
    case LightEffect::Twinkle:
      twinkle(params, step);
      break;
    case LightEffect::Sparkle:
      sparkle(params, step);
      break;
    case LightEffect::Transparent:
      break;
  }
}

void Lights::setRegionPixel(std::string regionName, uint32_t index, Color pixel) {
  auto region = lightsConfig->regions[regionName];
  if (index > region.count)
    return;

  uint32_t base = 0;
  for (int i = 0; i < region.breaks.size(); i++) {
    if (index <= base + region.breaks[i]) {
      auto& section = region.sections[i];
      uint16_t regionIndex = index - base;
      leds.setPixel(section.channel, pixel, regionIndex);
      break;
    }
    base += region.breaks[i];
  }
}

Color Lights::getRegionPixel(std::string regionName, uint32_t index) {
  auto region = lightsConfig->regions[regionName];
  if (index > region.count)
    return lightOff;

  uint32_t base = 0;
  for (int i = 0; i < region.breaks.size(); i++) {
    if (index <= base + region.breaks[i]) {
      auto& section = region.sections[i];
      uint16_t regionIndex = index - base;
      return leds.getPixel(section.channel, regionIndex);
    }
    base += region.breaks[i];
  }

  return lightOff;
}

Color Lights::blend(Color first, Color second, float weight) {
  Color blended;

  blended.r = (uint8_t)(round(first.r * weight) + round(second.r * (1.f - weight)));
  blended.g = (uint8_t)(round(first.g * weight) + round(second.g * (1.f - weight)));
  blended.b = (uint8_t)(round(first.b * weight) + round(second.b * (1.f - weight)));

  return blended;
}

Color Lights::getStepColor(RenderStep *step, ColorOption option) {
  if (option.random)
    return Color(random() % 255, random() % 255, random() % 255);
  else if (option.rainbow)
    return colorWheel(step->step % 256);
  else
    return option.color;
}

void Lights::color(LightingParameters *params, RenderStep *step) {
  auto first = getStepColor(step, params->first);
  colorRegion(params->region, first);

  // set time to render next frame
  step->next = REFRESH_NEVER;
}

void Lights::blink(LightingParameters *params, RenderStep *step) {
  // set color depending on odd or even step
  auto first = getStepColor(step, params->first);
  auto second = getStepColor(step, params->second);

  step->step % 2 == 0 ? colorRegion(params->region, first) : colorRegion(params->region, second);

  // set time to render next frame
  step->next = millis() + params->duration;
  step->step++;
}

void Lights::colorWipe(LightingParameters *params, RenderStep *step) {
  auto region = lightsConfig->regions[params->region];
  auto total = region.count;

  auto first = getStepColor(step, params->first);
  auto second = getStepColor(step, params->second);

  // the animation is complete
  if (step->step > total * 2) {
    step->next = REFRESH_NEVER;
    return;
  }
  
  uint8_t position = step->step > total ? step->step % total : step->step;

  if (step->step < total)
    setRegionPixel(params->region, position, first);
  else
    setRegionPixel(params->region, position, second);
  
  // calculate next animation step
  unsigned long next = params->duration / (total * 2);
  step->next = millis() + next;
  step->step++;
}

void Lights::breathe(LightingParameters *params, RenderStep *step) {
  uint16_t lum = step->step % 512;
  if (lum > 255) lum = 511 - lum;
  float weight = lum / 256.f;
  
  uint16_t delay;
  if(lum == 15) delay = 970;
  else if(lum <=  25) delay = 38;
  else if(lum <=  50) delay = 36;
  else if(lum <=  75) delay = 28;
  else if(lum <= 100) delay = 20;
  else if(lum <= 125) delay = 14;
  else if(lum <= 150) delay = 11;
  else delay = 10;

  auto first = getStepColor(step, params->first);
  auto second = getStepColor(step, params->second);
  auto color = blend(second, first, weight);
  colorRegion(params->region, color);

  step->step += 2;
  if (step->step > 512 - 15)
    step->step = 15;

  step->next = millis() + delay;
}

void Lights::fade(LightingParameters *params, RenderStep *step) {
  uint16_t lum = step->step % 512;
  if (lum > 255) lum = 511 - lum;
  float weight = lum / 256.f;

  auto first = getStepColor(step, params->first);
  auto second = getStepColor(step, params->second);
  auto color = blend(first, second, weight);
  colorRegion(params->region, color);

  step->step += 4;
  if (step->step > 511)
    step->step = 0;

  step->next = millis() + params->duration / 128;
}

void Lights::scan(LightingParameters *params, RenderStep *step) {
  auto region = lightsConfig->regions[params->region];
  auto first = getStepColor(step, params->first);
  auto second = getStepColor(step, params->second);

  bool direction;
  memcpy(&direction, step->data, sizeof(bool));

  colorRegion(params->region, first);
  setRegionPixel(params->region, step->step, second);

  step->step += direction ? 1 : -1;

  // swap directions when we hit the end
  if (step->step == 0 || step->step >= region.count)
    direction = !direction;

  memcpy(step->data, &direction, sizeof(bool));
  step->next = millis() + params->duration / (region.count * 2);
}

void Lights::rainbow(LightingParameters *params, RenderStep *step) {
  auto region = lightsConfig->regions[params->region];
  uint8_t position = step->step % 256;

  for (uint32_t i = 0; i < region.count; i++) {
    auto color = colorWheel(((i * 256 / region.count) + position) & 0xFF);
    setRegionPixel(params->region, i, color);
  }
  
  step->next = millis() + params->duration / 256;
  step->step++;
}

void Lights::rainbowCycle(LightingParameters *params, RenderStep *step) {
  uint8_t position = step->step % 256;
  auto color = colorWheel(position);
  colorRegion(params->region, color);

  step->next = millis() + params->duration / 256;
  step->step++;
}

void Lights::colorChase(LightingParameters *params, RenderStep *step) {
  auto region = lightsConfig->regions[params->region];
  auto first = getStepColor(step, params->first);
  auto second = getStepColor(step, params->second);
  auto third = getStepColor(step, params->third);
  
  uint8_t index = step->step % 3;
  for (uint32_t i = 0; i < region.count; i++, index++) {
    index %= 3;
    Color color;
    switch (index) {
      case 0: color = first; break;
      case 1: color = second; break;
      case 2: color = third; break;
      default: color = first;
    }
    setRegionPixel(params->region, i, color);
  }

  step->next = millis() + params->duration / 3;
  step->step++;
}

void Lights::theaterChase(LightingParameters *params, RenderStep *step) {
  auto region = lightsConfig->regions[params->region];
  bool on = step->step % 2 == 0;
  auto first = getStepColor(step, params->first);

  for (uint32_t i = step->step % 3; i < region.count; i += 3) {
    if (on) setRegionPixel(params->region, i, first);
    else setRegionPixel(params->region, i, lightOff);
  }

  step->next = millis() + params->duration;
  step->step++;
}

void Lights::twinkle(LightingParameters *params, RenderStep *step) {
  auto region = lightsConfig->regions[params->region];

  auto first = getStepColor(step, params->first);
  auto second = getStepColor(step, params->second);

  if (step->step == 0) {
    colorRegion(params->region, second);
    uint32_t min = (region.count / 4) + 1;
    step->step = rand() % min + min;
  }

  setRegionPixel(params->region, rand() % region.count, first);
  step->step--;

  step->next = millis() + region.count > 0 ? params->duration / region.count : REFRESH_NEVER;
}

void Lights::sparkle(LightingParameters *params, RenderStep *step) {
  auto region = lightsConfig->regions[params->region];
  auto first = getStepColor(step, params->first);
  auto second = getStepColor(step, params->second);

  uint32_t pixel;

  if (step->step == 0)
    colorRegion(params->region, first);
  else {
    memcpy(&pixel, step->data, sizeof(uint32_t));
    setRegionPixel(params->region, pixel, first);
  }

  pixel = region.count > 0 ? rand() % region.count : 0;
  memcpy(step->data, &pixel, sizeof(uint32_t));
  setRegionPixel(params->region, pixel, second);

  step->next = millis() + region.count > 0 ? params->duration / region.count : REFRESH_NEVER;
  step->step++;
}

void Lights::alternate(LightingParameters *params, RenderStep *step) {
  auto region = lightsConfig->regions[params->region];
  auto first = getStepColor(step, params->first);
  auto second = getStepColor(step, params->second);

  bool toggle = step->step % 2 == 0;
  for (uint32_t i = 0; i < region.count; i++) {
    setRegionPixel(params->region, i, toggle ? first : second);
    toggle = !toggle;
  }
  
  step->next = millis() + params->duration;
  step->step++;
}