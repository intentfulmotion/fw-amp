#include <hal/amp-1.0.0/amp-leds.h>

void AmpLeds::init() {
  // setup the status led
  status = new OneWireLED(WS2812B, STATUS_LED, 0, 1);
  (*status)[0] = lightOff;
}

void AmpLeds::deinit() {
  delay(50);
}

void AmpLeds::process() {
  if (dirty) {
    status->show();
    for (auto pair : channels) {
      if (pair.second != nullptr)
        pair.second->show();
    }
  }
  // if (statusDirty) {
  //   ESP_LOGV(LEDS_TAG,"Status is dirty. Re-rendering");
  //   status->wait();
  //   status->show();
  //   statusDirty = false;
  // }

  // // check the dirty bit for each
  // for (auto pair : channels) {
  //   if (pair.second != nullptr && dirty[pair.first]) {
  //     ESP_LOGV(LEDS_TAG,"Channel %d is dirty. Re-rendering", pair.first);
  //     pair.second->wait();
  //     pair.second->show();

  //     // unset dirty bit
  //     dirty[pair.first] = false;
  //   }
  // }
}

LightController* AmpLeds::addLEDStrip(LightChannel data) {
  ESP_LOGD(LEDS_TAG,"Adding %s strip on channel %d with %d LEDs", data.type <= 1 ? "Neopixel" : "DotStar", data.channel, data.leds);
  AddressableLED *controller = nullptr;

  switch(data.type) {
    case LEDType::NeoPixel:
    case LEDType::WS2812:
    case LEDType::WS2812B:
    case LEDType::WS2813:
    case LEDType::SK6812:
      controller = new OneWireLED(data.type, lightMap[data.channel], data.channel, data.leds);
      break;
    case LEDType::SK6812_RGBW:
      controller = new OneWireLED(data.type, lightMap[data.channel], data.channel, data.leds, PixelOrder::RGBW);
      break;
    case LEDType::DotStar:
    case LEDType::APA102:
      controller = new TwoWireLED(HSPI_HOST, data.leds, lightMap[data.channel], lightMap[data.channel + 4]);
      break;
    default:
      return nullptr;
  }

  for (uint16_t i = 0; i < data.leds; i++)
    (*controller)[i] = lightOff;

  channels[data.channel] = controller;
  // dirty[data.channel] = true;

  return controller;
}

void AmpLeds::setStatus(Color color) {
  (*status)[0] = gammaCorrected(color);
  dirty = true;
  // statusDirty = true;
}

void AmpLeds::render(bool all, int8_t channel) {
  dirty = true;
  // statusDirty = true;

  // // set all dirty bits
  // if (all) {
  //   for (auto pair : dirty)
  //     dirty[pair.first] = true;
  // }
  // else if (channel != -1 && channel >= 1 && channel <= 8)
  //   dirty[channel] = true;
}

Color AmpLeds::gammaCorrected(Color color) {
  return Color(gamma8[color.r], gamma8[color.g], gamma8[color.b]);
}

void AmpLeds::setPixel(uint8_t channelNumber, Color color, uint16_t index) {
  auto controller = channels[channelNumber];
  if (controller == nullptr)
    return;

  (*controller)[index] = color;
}

void AmpLeds::setPixels(uint8_t channelNumber, Color color, uint16_t start, uint16_t end){
  auto controller = channels[channelNumber];
  if (controller == nullptr)
    return;
  
  for (uint16_t i = start - 1; i < end; i++)
    (*controller)[i] = color;
}