#include <hal/amp-1.0.0/amp-leds.h>

void AmpLeds::init() {
  // setup the status led
  FastLED.addLeds<NEOPIXEL, STATUS_LED>(statusData, 1);
  statusData[0] = CRGB::Black;

  FastLED.setMaxPowerInMilliWatts(6200);
}

void AmpLeds::deinit() {
  FastLED.showColor(lightOff);

  delay(50);
}

void AmpLeds::process() {
  if (dirty) {
    FastLED.show();
    dirty = false;
  }
}

LightController* AmpLeds::addLEDStrip(LightChannel data) {
  Log::trace("Adding %s strip on channel %d with %d LEDs", data.type == 0 ? "Neopixel" : "DotStar", data.channel, data.leds);
  auto channel = data.channel;
  switch (channel) {
    case 1:
      channelOneData = new CRGB[data.leds];
      if (data.type == 0)
        channels[channel] = &FastLED.addLeds<NEOPIXEL, STRIP_ONE_DATA>(channelOneData, data.leds);
      else if (data.type == 1)
        channels[channel] = &FastLED.addLeds<DOTSTAR, STRIP_ONE_DATA, STRIP_ONE_CLK>(channelOneData, data.leds);
      break;
    case 2:
      channelTwoData = new CRGB[data.leds];
      if (data.type == 0)
        channels[channel] = &FastLED.addLeds<NEOPIXEL, STRIP_TWO_DATA>(channelTwoData, data.leds);
      else if (data.type == 1)
        channels[channel] = &FastLED.addLeds<DOTSTAR, STRIP_TWO_DATA, STRIP_TWO_CLK>(channelTwoData, data.leds);      
      break;
    case 3:
      channelThreeData = new CRGB[data.leds];
      if (data.type == 0)
        channels[channel] = &FastLED.addLeds<NEOPIXEL, STRIP_THREE_DATA>(channelThreeData, data.leds);
      else if (data.type == 1)
        channels[channel] = &FastLED.addLeds<DOTSTAR, STRIP_THREE_DATA, STRIP_THREE_CLK>(channelThreeData, data.leds); 
      break;
    case 4:
      channelFourData = new CRGB[data.leds];
      if (data.type == 0)
        channels[channel] = &FastLED.addLeds<NEOPIXEL, STRIP_FOUR_DATA>(channelFourData, data.leds);
      else if (data.type == 1)
        channels[channel] = &FastLED.addLeds<DOTSTAR, STRIP_FOUR_DATA, STRIP_FOUR_CLK>(channelFourData, data.leds); 
      break;
    case 5:
      channelFiveData = new CRGB[data.leds];
      if (data.type == 0)
        channels[channel] = &FastLED.addLeds<NEOPIXEL, STRIP_ONE_CLK>(channelFiveData, data.leds);
      break;
    case 6:
      channelSixData = new CRGB[data.leds];
      if (data.type == 0)
        channels[channel] = &FastLED.addLeds<NEOPIXEL, STRIP_TWO_CLK>(channelSixData, data.leds);
      break;
    case 7:
      channelSevenData = new CRGB[data.leds];
      if (data.type == 0)
        channels[channel] = &FastLED.addLeds<NEOPIXEL, STRIP_THREE_CLK>(channelSevenData, data.leds);
      break;
    case 8:
      channelEightData = new CRGB[data.leds];
      if (data.type == 0)
        channels[channel] = &FastLED.addLeds<NEOPIXEL, STRIP_FOUR_CLK>(channelEightData, data.leds);
      break;
  }

  return channels[channel];
}

void AmpLeds::setStatus(Color color) {
  statusData[0] = gammaCorrected(color);
  dirty = true;
}

void AmpLeds::render() {
  dirty = true;
}

Color AmpLeds::gammaCorrected(Color color) {
  return Color(gamma8[color.r], gamma8[color.g], gamma8[color.b]);
}