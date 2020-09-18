#pragma once

#include <common.h>
#include <map>

#include <models/light.h>
#include "light-controller.h"

class AmpLeds {
  ColorRGB statusData[1];
  ColorRGB *channelOneData;
  ColorRGB *channelTwoData;
  ColorRGB *channelThreeData;
  ColorRGB *channelFourData;
  ColorRGB *channelFiveData;
  ColorRGB *channelSixData;
  ColorRGB *channelSevenData;
  ColorRGB *channelEightData;

  LightController *status;
  std::map<uint8_t, LightController*> channels;
  std::map<uint8_t, uint16_t> leds;

  std::map<uint8_t, uint8_t> lightMap {
    std::make_pair(1, STRIP_ONE_DATA),
    std::make_pair(2, STRIP_TWO_DATA),
    std::make_pair(3, STRIP_THREE_DATA),
    std::make_pair(4, STRIP_FOUR_DATA),
    // neopixels can give the option for 4 more channels
    std::make_pair(5, STRIP_ONE_CLK),
    std::make_pair(6, STRIP_TWO_CLK),
    std::make_pair(7, STRIP_THREE_CLK),
    std::make_pair(8, STRIP_FOUR_CLK),
  };

  uint8_t _brightness = 255;
  bool dirty;

  NeoGamma<NeoGammaTableMethod> colorGamma;

  public:
    void init();
    void deinit();
    void process();
    void setStatus(ColorRGB color);
    void render();
    ColorRGB gammaCorrected(ColorRGB color);
    ColorRGBW gammaCorrectedRGBW(ColorRGBW color);
    void setBrightness(uint8_t brightness) { _brightness = brightness; /*FastLED.setBrightness(_brightness);*/ }

    LightController* addLEDStrip(LightChannel data);
};