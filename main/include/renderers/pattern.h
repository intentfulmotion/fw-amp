#pragma once
#include <interfaces/renderer.h>

class PatternRenderer : public LoopingRenderer {
  std::map<uint8_t, LightChannel> channels;
  int totalLeds = 0;
  std::string _pattern;

  public:
    PatternRenderer(Lights *lights, std::string pattern) : LoopingRenderer(lights) {
      channels = lights->getAvailableChannels();
      _pattern = pattern;
      
      for (auto channel : channels)
        totalLeds += channel.second.leds;
    }

    void process();

    void allColorRGB(ColorRGB color);
    void allRandom();
    void dissolve(int simultaneous, int cycles, int speed);
    void rainbow(int cycles, int speed);
    void theaterChase(ColorRGB c, int cycles, int speed);
    void theaterChaseRainbow(int cycles, int speed);
    void lightning(int simultaneous, int cycles, int speed);
};