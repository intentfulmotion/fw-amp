#include <hal/amp-1.0.0/amp-leds.h>

void AmpLeds::init() {
  status = new LightController(StripType::NeoPixel_GRB, 1, STATUS_LED);

  // FastLED.setMaxPowerInMilliWatts(6200);
}

void AmpLeds::deinit() {
  // FastLED.showColorRGB(lightOff);

  delay(50);
}

void AmpLeds::process() {
  status->Show();

  for (auto channelItem : channels) {
    auto channel = channelItem.second;
    channel->Show();
  }
}

LightController* AmpLeds::addLEDStrip(LightChannel data) {
  Log::trace("Adding %d strip on channel %d with %d LEDs", data.type, data.channel, data.leds);
  auto channel = data.channel;
  int pin = 0;

  switch (channel) {
    case 1:
      pin = STRIP_ONE_DATA;
      break;
    case 2:
      pin = STRIP_TWO_DATA;
      break;
    case 3:
      pin = STRIP_THREE_DATA;
      break;
    case 4:
      pin = STRIP_FOUR_DATA;
      break;
    case 5:
      pin = STRIP_ONE_CLK;
      break;
    case 6:
      pin = STRIP_TWO_CLK;
      break;
    case 7:
      pin = STRIP_THREE_CLK;
      break;
    case 8:
      pin = STRIP_FOUR_CLK;
      break;
    default:
      pin = 0;
  }

  // ensures valid pin and that we don't set a DotStar on channels 5-8
  if (pin == 0 || (data.type >= DotStar_BGR && channel >= 5))
    return nullptr;

  channels[channel] = new LightController(data.type, data.leds, pin);

  return channels[channel];
}

void AmpLeds::setStatus(ColorRGB color) {
  status->SetPixelColor(0, color);
}

void AmpLeds::render() {
  dirty = true;
}

ColorRGB AmpLeds::gammaCorrected(ColorRGB color) {
  return colorGamma.Correct(color);
}

ColorRGBW AmpLeds::gammaCorrectedRGBW(ColorRGBW color) {
  return colorGamma.Correct(color);
}