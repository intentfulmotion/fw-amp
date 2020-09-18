#include "light-controller.h"

LightController::LightController(StripType type, uint16_t countPixels, uint8_t pin) {
  _type = type;

  switch (_type) {
    case StripType::NeoPixel_GRB:
      createNeoPixelGRBForPin(countPixels, pin);

      n_grb->Begin();
      n_grb->Show();
      break;
    case StripType::NeoPixel_GRBW:
      createNeoPixelGRBWForPin(countPixels, pin);

      n_grbw->Begin();
      n_grbw->Show();
      break;
    case StripType::NeoPixel_RGB:
      createNeoPixelRGBForPin(countPixels, pin);

      n_rgb->Begin();
      n_rgb->Show();
      break;
    case StripType::NeoPixel_RGBW:
      createNeoPixelRGBWForPin(countPixels, pin);

      n_rgbw->Begin();
      n_rgbw->Show();
      break;
    
    // todo: add DotStar / SK6812 options
  }
}

void LightController::createNeoPixelGRBForPin(uint16_t countPixels, uint8_t pin) {
  switch (pin) {
    case 1:
      n_grb = new NeoPixelBus<NeoGrbFeature, NeoEsp32Rmt0800KbpsMethod>(countPixels, pin);
      break;
    case 2:
      n_grb = new NeoPixelBus<NeoGrbFeature, NeoEsp32Rmt1800KbpsMethod>(countPixels, pin);
      break;
    case 3:
      n_grb = new NeoPixelBus<NeoGrbFeature, NeoEsp32Rmt2800KbpsMethod>(countPixels, pin);
      break;
    case 4:
      n_grb = new NeoPixelBus<NeoGrbFeature, NeoEsp32Rmt3800KbpsMethod>(countPixels, pin);
      break;
    case 5:
      n_grb = new NeoPixelBus<NeoGrbFeature, NeoEsp32Rmt4800KbpsMethod>(countPixels, pin);
      break;
    case 6:
      n_grb = new NeoPixelBus<NeoGrbFeature, NeoEsp32Rmt5800KbpsMethod>(countPixels, pin);
      break;
    case 7:
      n_grb = new NeoPixelBus<NeoGrbFeature, NeoEsp32Rmt6800KbpsMethod>(countPixels, pin);
      break;
    case 8:
      n_grb = new NeoPixelBus<NeoGrbFeature, NeoEsp32Rmt7800KbpsMethod>(countPixels, pin);
      break;
    default: break;
  }
}

void LightController::createNeoPixelGRBWForPin(uint16_t countPixels, uint8_t pin) {
  switch (pin) {
    case 1:
      n_grb = new NeoPixelBus<NeoGrbwFeature, NeoEsp32Rmt0800KbpsMethod>(countPixels, pin);
      break;
    case 2:
      n_grb = new NeoPixelBus<NeoGrbwFeature, NeoEsp32Rmt1800KbpsMethod>(countPixels, pin);
      break;
    case 3:
      n_grb = new NeoPixelBus<NeoGrbwFeature, NeoEsp32Rmt2800KbpsMethod>(countPixels, pin);
      break;
    case 4:
      n_grb = new NeoPixelBus<NeoGrbwFeature, NeoEsp32Rmt3800KbpsMethod>(countPixels, pin);
      break;
    case 5:
      n_grb = new NeoPixelBus<NeoGrbwFeature, NeoEsp32Rmt4800KbpsMethod>(countPixels, pin);
      break;
    case 6:
      n_grb = new NeoPixelBus<NeoGrbwFeature, NeoEsp32Rmt5800KbpsMethod>(countPixels, pin);
      break;
    case 7:
      n_grb = new NeoPixelBus<NeoGrbwFeature, NeoEsp32Rmt6800KbpsMethod>(countPixels, pin);
      break;
    case 8:
      n_grb = new NeoPixelBus<NeoGrbwFeature, NeoEsp32Rmt7800KbpsMethod>(countPixels, pin);
      break;
    default: break;
  }
}

void LightController::createNeoPixelRGBForPin(uint16_t countPixels, uint8_t pin) {
  switch (pin) {
    case 1:
      n_grb = new NeoPixelBus<NeoRgbFeature, NeoEsp32Rmt0800KbpsMethod>(countPixels, pin);
      break;
    case 2:
      n_grb = new NeoPixelBus<NeoRgbFeature, NeoEsp32Rmt1800KbpsMethod>(countPixels, pin);
      break;
    case 3:
      n_grb = new NeoPixelBus<NeoRgbFeature, NeoEsp32Rmt2800KbpsMethod>(countPixels, pin);
      break;
    case 4:
      n_grb = new NeoPixelBus<NeoRgbFeature, NeoEsp32Rmt3800KbpsMethod>(countPixels, pin);
      break;
    case 5:
      n_grb = new NeoPixelBus<NeoRgbFeature, NeoEsp32Rmt4800KbpsMethod>(countPixels, pin);
      break;
    case 6:
      n_grb = new NeoPixelBus<NeoRgbFeature, NeoEsp32Rmt5800KbpsMethod>(countPixels, pin);
      break;
    case 7:
      n_grb = new NeoPixelBus<NeoRgbFeature, NeoEsp32Rmt6800KbpsMethod>(countPixels, pin);
      break;
    case 8:
      n_grb = new NeoPixelBus<NeoRgbFeature, NeoEsp32Rmt7800KbpsMethod>(countPixels, pin);
      break;
    default: break;
  }
}

void LightController::createNeoPixelRGBWForPin(uint16_t countPixels, uint8_t pin) {
  switch (pin) {
    case 1:
      n_grb = new NeoPixelBus<NeoRgbwFeature, NeoEsp32Rmt0800KbpsMethod>(countPixels, pin);
      break;
    case 2:
      n_grb = new NeoPixelBus<NeoRgbwFeature, NeoEsp32Rmt1800KbpsMethod>(countPixels, pin);
      break;
    case 3:
      n_grb = new NeoPixelBus<NeoRgbwFeature, NeoEsp32Rmt2800KbpsMethod>(countPixels, pin);
      break;
    case 4:
      n_grb = new NeoPixelBus<NeoRgbwFeature, NeoEsp32Rmt3800KbpsMethod>(countPixels, pin);
      break;
    case 5:
      n_grb = new NeoPixelBus<NeoRgbwFeature, NeoEsp32Rmt4800KbpsMethod>(countPixels, pin);
      break;
    case 6:
      n_grb = new NeoPixelBus<NeoRgbwFeature, NeoEsp32Rmt5800KbpsMethod>(countPixels, pin);
      break;
    case 7:
      n_grb = new NeoPixelBus<NeoRgbwFeature, NeoEsp32Rmt6800KbpsMethod>(countPixels, pin);
      break;
    case 8:
      n_grb = new NeoPixelBus<NeoRgbwFeature, NeoEsp32Rmt7800KbpsMethod>(countPixels, pin);
      break;
    default: break;
  }
}