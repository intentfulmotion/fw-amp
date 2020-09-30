#pragma once

#include <common.h>
#include <map>

#include <models/light.h>

#include <OneWireLED.h>
#include <TwoWireLED.h>

const uint8_t gamma8[] = {
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,
    1,  1,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,
    2,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,  4,  4,  5,  5,  5,
    5,  6,  6,  6,  6,  7,  7,  7,  7,  8,  8,  8,  9,  9,  9, 10,
   10, 10, 11, 11, 11, 12, 12, 13, 13, 13, 14, 14, 15, 15, 16, 16,
   17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 24, 24, 25,
   25, 26, 27, 27, 28, 29, 29, 30, 31, 32, 32, 33, 34, 35, 35, 36,
   37, 38, 39, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 50,
   51, 52, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 66, 67, 68,
   69, 70, 72, 73, 74, 75, 77, 78, 79, 81, 82, 83, 85, 86, 87, 89,
   90, 92, 93, 95, 96, 98, 99,101,102,104,105,107,109,110,112,114,
  115,117,119,120,122,124,126,127,129,131,133,135,137,138,140,142,
  144,146,148,150,152,154,156,158,160,162,164,167,169,171,173,175,
  177,180,182,184,186,189,191,193,196,198,200,203,205,208,210,213,
  215,218,220,223,225,228,231,233,236,239,241,244,247,249,252,255 };

static const char* LEDS_TAG = "leds";

class AmpLeds {
  LightController* status;
  std::map<uint8_t, LightController*> channels;
  std::map<uint8_t, uint16_t> leds;
  // std::map<uint8_t, bool> dirty;

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
  // bool statusDirty = false;
  bool dirty;

  NeoGamma<NeoGammaTableMethod> colorGamma;

  public:
    void init();
    void deinit();
    void process();
    void setStatus(Color color);
    void render(bool all = false, int8_t channel = -1);
    Color gammaCorrected(Color color);
    void setBrightness(uint8_t brightness) { _brightness = brightness; /*FastLED.setBrightness(_brightness);*/ }

    void setPixel(uint8_t channelNumber, Color color, uint16_t index);
    void setPixels(uint8_t channelNumber, Color color, uint16_t start, uint16_t end);

    LightController* addLEDStrip(LightChannel data);
};