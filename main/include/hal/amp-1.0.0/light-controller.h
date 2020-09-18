#include <models/light.h>
#include <NeoPixelBus.h>
#include <NeoPixelAnimator.h>
#include <NeoEsp32I2sMethod.h>
#include <NeoEsp32RmtMethod.h>

class LightController {
  private:
    StripType _type;
    NeoPixelBus<NeoRgbFeature, NeoEsp32I2s1800KbpsMethod>* n_rgb;
    NeoPixelBus<NeoRgbwFeature, NeoEsp32I2s1800KbpsMethod>* n_rgbw;
    NeoPixelBus<NeoGrbFeature, NeoEsp32I2s1800KbpsMethod>* n_grb;
    NeoPixelBus<NeoGrbwFeature, NeoEsp32I2s1800KbpsMethod>* n_grbw;
    // NeoPixelBus<DotStarBgrFeature, DotStarS

  public:
    LightController(StripType type, uint16_t countPixels, uint8_t pin);
    
    ~LightController() { cleanup(); }

    void createNeoPixelGRBForPin(uint16_t countPixels, uint8_t pin);
    void createNeoPixelGRBWForPin(uint16_t countPixels, uint8_t pin);
    void createNeoPixelRGBForPin(uint16_t countPixels, uint8_t pin);
    void createNeoPixelRGBWForPin(uint16_t countPixels, uint8_t pin);

    bool CanShow() {
      switch (_type) {
        case StripType::NeoPixel_GRB:
          return n_grb->CanShow();
        case StripType::NeoPixel_GRBW:
          return n_grbw->CanShow();
        case StripType::NeoPixel_RGB:
          return n_rgb->CanShow();
        case StripType::NeoPixel_RGBW:
          return n_rgbw->CanShow();
        default:
          return false;
      }
    }

    void Show(bool maintainBufferConsistency = true) {
      switch (_type) {
        case StripType::NeoPixel_GRB:
          n_grb->Show(maintainBufferConsistency);
          break;
        case StripType::NeoPixel_GRBW:
          n_grbw->Show(maintainBufferConsistency);
          break;
        case StripType::NeoPixel_RGB:
          n_rgb->Show(maintainBufferConsistency);
          break;
        case StripType::NeoPixel_RGBW:
          n_rgbw->Show(maintainBufferConsistency);
          break;
        default:
          break;
      }
    }

    bool IsDirty() {
      switch (_type) {
        case StripType::NeoPixel_GRB:
          return n_grb->IsDirty();
        case StripType::NeoPixel_GRBW:
          return n_grbw->IsDirty();
        case StripType::NeoPixel_RGB:
          return n_rgb->IsDirty();
        case StripType::NeoPixel_RGBW:
          return n_rgbw->IsDirty();
        default:
          return false;
      }
    }

    void Dirty() {
      switch (_type) {
        case StripType::NeoPixel_GRB:
          n_grb->Dirty();
          break;
        case StripType::NeoPixel_GRBW:
          n_grbw->Dirty();
          break;
        case StripType::NeoPixel_RGB:
          n_rgb->Dirty();
          break;
        case StripType::NeoPixel_RGBW:
          n_rgbw->Dirty();
          break;
        default:
          break;
      }
    }

    void ResetDirty() {
      switch (_type) {
        case StripType::NeoPixel_GRB:
          n_grb->ResetDirty();
          break;
        case StripType::NeoPixel_GRBW:
          n_grbw->ResetDirty();
          break;
        case StripType::NeoPixel_RGB:
          n_rgb->ResetDirty();
          break;
        case StripType::NeoPixel_RGBW:
          n_rgbw->ResetDirty();
          break;
        default:
          break;
      }
    }

    uint8_t* Pixels() {
      switch (_type) {
        case StripType::NeoPixel_GRB:
          return n_grb->Pixels();
        case StripType::NeoPixel_GRBW:
          return n_grbw->Pixels();
        case StripType::NeoPixel_RGB:
          return n_rgb->Pixels();
        case StripType::NeoPixel_RGBW:
          return n_rgbw->Pixels();
        default:
          return 0;
      }
    }

    size_t PixelsSize() {
      switch (_type) {
        case StripType::NeoPixel_GRB:
          return n_grb->PixelsSize();
        case StripType::NeoPixel_GRBW:
          return n_grbw->PixelsSize();
        case StripType::NeoPixel_RGB:
          return n_rgb->PixelsSize();
        case StripType::NeoPixel_RGBW:
          return n_rgbw->PixelsSize();
        default:
          return 0;
      }
    }

    size_t PixelSize() {
      switch (_type) {
        case StripType::NeoPixel_GRB:
          return n_grb->PixelSize();
        case StripType::NeoPixel_GRBW:
          return n_grbw->PixelSize();
        case StripType::NeoPixel_RGB:
          return n_rgb->PixelSize();
        case StripType::NeoPixel_RGBW:
          return n_rgbw->PixelSize();
        default:
          return 0;
      }
    }

    uint16_t PixelCount() {
      switch (_type) {
        case StripType::NeoPixel_GRB:
          return n_grb->PixelCount();
        case StripType::NeoPixel_GRBW:
          return n_grbw->PixelCount();
        case StripType::NeoPixel_RGB:
          return n_rgb->PixelCount();
        case StripType::NeoPixel_RGBW:
          return n_rgbw->PixelCount();
        default:
          return 0;
      }
    }

    void SetPixelColor(uint16_t indexPixel, RgbColor color) {
      switch (_type) {
        case StripType::NeoPixel_GRB:
          n_grb->SetPixelColor(indexPixel, color);
          break;
        case StripType::NeoPixel_GRBW:
          break;
        case StripType::NeoPixel_RGB:
          n_rgb->SetPixelColor(indexPixel, color);
          break;
        case StripType::NeoPixel_RGBW:
          n_rgbw->ResetDirty();
          break;
        default:
          break;
      }
    }

    void SetPixelColor(uint16_t indexPixel, RgbwColor color) {
      switch (_type) {
        case StripType::NeoPixel_GRB:
          break;
        case StripType::NeoPixel_GRBW:
          n_grbw->SetPixelColor(indexPixel, color);
          break;
        case StripType::NeoPixel_RGB:
          break;
        case StripType::NeoPixel_RGBW:
          n_rgbw->SetPixelColor(indexPixel, color);
          break;
        default:
          break;
      }
    }

    RgbColor GetPixelColor(uint16_t indexPixel) {
      switch (_type) {
        case StripType::NeoPixel_GRB:
          return n_grb->GetPixelColor(indexPixel);
        case StripType::NeoPixel_GRBW:
          return 0;
        case StripType::NeoPixel_RGB:
          return n_rgb->GetPixelColor(indexPixel);
          break;
        case StripType::NeoPixel_RGBW:
          return 0;
        default:
          return 0;
      }
    }

    RgbwColor GetPixelColorRgbw(uint16_t indexPixel) {
      switch (_type) {
        case StripType::NeoPixel_GRB:
          return 0;
        case StripType::NeoPixel_GRBW:
          n_grbw->GetPixelColor(indexPixel);
          break;
        case StripType::NeoPixel_RGB:
          return 0;
        case StripType::NeoPixel_RGBW:
          n_rgbw->GetPixelColor(indexPixel);
          break;
        default:
          return 0;
      }
    }

    void ClearTo(RgbColor color)
    {
      switch (_type) {
        case StripType::NeoPixel_GRB:
          n_grb->ClearTo(color);
          break;
        case StripType::NeoPixel_GRBW:
          break;
        case StripType::NeoPixel_RGB:
          n_rgb->ClearTo(color);
          break;
        case StripType::NeoPixel_RGBW:
          break;
        default:
          break;
      }
    }

    void ClearTo(RgbwColor color)
    {
      switch (_type) {
        case StripType::NeoPixel_GRB:
          break;
        case StripType::NeoPixel_GRBW:
          n_grbw->ClearTo(color);
          break;
        case StripType::NeoPixel_RGB:
          break;
        case StripType::NeoPixel_RGBW:
          n_rgbw->ClearTo(color);
          break;
        default:
          break;
      }
    }

    void ClearTo(RgbColor color, uint16_t first, uint16_t last)
    {
      switch (_type) {
        case StripType::NeoPixel_GRB:
          n_grb->ClearTo(color, first, last);
          break;
        case StripType::NeoPixel_GRBW:
          break;
        case StripType::NeoPixel_RGB:
          n_rgb->ClearTo(color, first, last);
          break;
        case StripType::NeoPixel_RGBW:
          break;
        default:
          break;
      }
    }

    void ClearTo(RgbwColor color, uint16_t first, uint16_t last)
    {
      switch (_type) {
        case StripType::NeoPixel_GRB:
          break;
        case StripType::NeoPixel_GRBW:
          n_grbw->ClearTo(color, first, last);
          break;
        case StripType::NeoPixel_RGB:
          break;
        case StripType::NeoPixel_RGBW:
          n_rgbw->ClearTo(color, first, last);
          break;
        default:
          break;
      }
    }

    void RotateLeft(uint16_t rotationCount)
    {
      switch (_type) {
        case StripType::NeoPixel_GRB:
          n_grb->RotateLeft(rotationCount);
          break;
        case StripType::NeoPixel_GRBW:
          n_grbw->RotateLeft(rotationCount);
          break;
        case StripType::NeoPixel_RGB:
          n_rgb->RotateLeft(rotationCount);
          break;
        case StripType::NeoPixel_RGBW:
          n_rgbw->RotateLeft(rotationCount);
          break;
        default:
          break;
      }
    }

    void RotateLeft(uint16_t rotationCount, uint16_t first, uint16_t last)
    {
      switch (_type) {
        case StripType::NeoPixel_GRB:
          n_grb->RotateLeft(rotationCount, first, last);
          break;
        case StripType::NeoPixel_GRBW:
          n_grbw->RotateLeft(rotationCount, first, last);
          break;
        case StripType::NeoPixel_RGB:
          n_rgb->RotateLeft(rotationCount, first, last);
          break;
        case StripType::NeoPixel_RGBW:
          n_rgbw->RotateLeft(rotationCount, first, last);
          break;
        default:
          break;
      }
    }

    void ShiftLeft(uint16_t shiftCount)
    {
      switch (_type) {
        case StripType::NeoPixel_GRB:
          n_grb->ShiftLeft(shiftCount);
          break;
        case StripType::NeoPixel_GRBW:
          n_grbw->ShiftLeft(shiftCount);
          break;
        case StripType::NeoPixel_RGB:
          n_rgb->ShiftLeft(shiftCount);
          break;
        case StripType::NeoPixel_RGBW:
          n_rgbw->ShiftLeft(shiftCount);
          break;
        default:
          break;
      }
    }

    void ShiftLeft(uint16_t shiftCount, uint16_t first, uint16_t last)
    {
      switch (_type) {
        case StripType::NeoPixel_GRB:
          n_grb->ShiftLeft(shiftCount, first, last);
          break;
        case StripType::NeoPixel_GRBW:
          n_grbw->ShiftLeft(shiftCount, first, last);
          break;
        case StripType::NeoPixel_RGB:
          n_rgb->ShiftLeft(shiftCount, first, last);
          break;
        case StripType::NeoPixel_RGBW:
          n_rgbw->ShiftLeft(shiftCount, first, last);
          break;
        default:
          break;
      }
    }

    void RotateRight(uint16_t rotationCount)
    {
      switch (_type) {
        case StripType::NeoPixel_GRB:
          n_grb->RotateRight(rotationCount);
          break;
        case StripType::NeoPixel_GRBW:
          n_grbw->RotateRight(rotationCount);
          break;
        case StripType::NeoPixel_RGB:
          n_rgb->RotateRight(rotationCount);
          break;
        case StripType::NeoPixel_RGBW:
          n_rgbw->RotateRight(rotationCount);
          break;
        default:
          break;
      }
    }

    void RotateRight(uint16_t rotationCount, uint16_t first, uint16_t last)
    {
      switch (_type) {
        case StripType::NeoPixel_GRB:
          n_grb->RotateRight(rotationCount, first, last);
          break;
        case StripType::NeoPixel_GRBW:
          n_grbw->RotateRight(rotationCount, first, last);
          break;
        case StripType::NeoPixel_RGB:
          n_rgb->RotateRight(rotationCount, first, last);
          break;
        case StripType::NeoPixel_RGBW:
          n_rgbw->RotateRight(rotationCount, first, last);
          break;
        default:
          break;
      }
    }

    void ShiftRight(uint16_t shiftCount)
    {
      switch (_type) {
        case StripType::NeoPixel_GRB:
          n_grb->ShiftRight(shiftCount);
          break;
        case StripType::NeoPixel_GRBW:
          n_grbw->ShiftRight(shiftCount);
          break;
        case StripType::NeoPixel_RGB:
          n_rgb->ShiftRight(shiftCount);
          break;
        case StripType::NeoPixel_RGBW:
          n_rgbw->ShiftRight(shiftCount);
          break;
        default:
          break;
      }
    }

    void ShiftRight(uint16_t shiftCount, uint16_t first, uint16_t last)
    {
      switch (_type) {
        case StripType::NeoPixel_GRB:
          n_grb->ShiftRight(shiftCount, first, last);
          break;
        case StripType::NeoPixel_GRBW:
          n_grbw->ShiftRight(shiftCount, first, last);
          break;
        case StripType::NeoPixel_RGB:
          n_rgb->ShiftRight(shiftCount, first, last);
          break;
        case StripType::NeoPixel_RGBW:
          n_rgbw->ShiftRight(shiftCount, first, last);
          break;
        default:
          break;
      }
    }

    void cleanup() {
      switch (_type) {
        case StripType::NeoPixel_GRB:
          delete n_grb;
          n_grb = NULL;
          break;
        case StripType::NeoPixel_GRBW:
          delete n_grbw;
          n_rgbw = NULL;
          break;
        case StripType::NeoPixel_RGB:
          delete n_rgb;
          n_rgb = NULL;
          break;
        case StripType::NeoPixel_RGBW:
          delete n_rgbw;
          n_rgbw = NULL;
          break;
      }
    }
};