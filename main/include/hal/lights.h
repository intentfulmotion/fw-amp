#pragma once
#include <vector>

#include <common.h>
#include <math.h>
#include <queue>
#include <interfaces/lifecycle.h>
#include <interfaces/power-listener.h>
#include <interfaces/touch-listener.h>
#include <interfaces/config-listener.h>
#include <interfaces/ble-listener.h>
#include <interfaces/calibration-listener.h>
#include <interfaces/update-listener.h>
#include <models/light.h>
#include <functional>

#if defined(AMP_1_0_x)
  #include <hal/amp-1.0.0/amp-leds.h>
  #include <hal/amp-1.0.0/amp-storage.h>
#endif

#include <hal/config.h>

#define REFRESH_NEVER   0

static const char* LIGHTS_TAG = "lights";

class Lights : public LifecycleBase,
  public PowerListener, public TouchListener, public ConfigListener, 
  public CalibrationListener, public UpdateListener, public BleListener {

  AmpLeds leds;
  LightsConfig *lightsConfig;
  bool init = false;
  bool calibratingXG = false;
  bool calibratingMag = false;
  bool safeToLight = false;
  TaskHandle_t calibrateLightHandle;

  bool updating = false;
  bool updateToggle = false;
  TaskHandle_t updateLightHandle;

  bool advertising = false;
  bool advertisingToggle = false;
  TaskHandle_t advertisingLightHandle;

  std::map<std::string, LightingParameters> _effects;
  std::map<std::string, RenderStep> _steps;
  std::map<std::string, uint32_t> _pixelCounts;

  static void renderer(void *args);
  void renderLightingEffect(LightingParameters *params, RenderStep *step);
  void color(LightingParameters *params, RenderStep *step);
  void blink(LightingParameters *params, RenderStep *step);
  void alternate(LightingParameters *params, RenderStep *step);
  void colorWipe(LightingParameters *params, RenderStep *step);
  void breathe(LightingParameters *params, RenderStep *step);
  void fade(LightingParameters *params, RenderStep *step);
  void scan(LightingParameters *params, RenderStep *step);
  void rainbow(LightingParameters *params, RenderStep *step);
  void rainbowCycle(LightingParameters *params, RenderStep *step);
  void colorChase(LightingParameters *params, RenderStep *step);
  void theaterChase(LightingParameters *params, RenderStep *step);
  void twinkle(LightingParameters *params, RenderStep *step);
  void sparkle(LightingParameters *params, RenderStep *step);

  TaskHandle_t renderHandle;

  unsigned long _lastRender = millis();

  void setRegionPixel(std::string regionName, uint32_t index, Color pixel);
  Color getRegionPixel(std::string regionName, uint32_t index);
  Color blend(Color first, Color second, float weight);

  void startEffect(LightingParameters parameters);
  void endEffect(LightingParameters parameters);

  Color getStepColor(RenderStep *step, ColorOption option);

  public:
    Lights();
    std::map<int, LightController*> controllers;
    static Lights* instance() { static Lights lights; return &lights; }
    
    // LifecycleBase
    void onPowerUp();
    void onPowerDown();

    // PowerListener
    void onPowerStatusChanged(PowerStatus status);
    void updateLightForPowerStatus(PowerStatus status);

    // TouchListener
    void onTouchEvent(std::vector<TouchType> *touches) { }
    void onTouchDown();
    void onTouchUp();

    // ConfigListener
    void onConfigUpdated();

    // CalibrationListener
    void onCalibrateXGStarted();
    void onCalibrateXGEnded();
    void onCalibrateMagStarted();
    void onCalibrateMagEnded();

    // UpdateListener
    void onUpdateStatusChanged(UpdateStatus status);

    // BleListener
    void onAdvertisingStarted();
    void onAdvertisingStopped();

    void process();

    uint16_t getLEDCountForChannel(uint8_t channel);
    LightRegion getLightRegion(std::string region);
    std::map<uint8_t, LightChannel> getAvailableChannels();
    std::map<std::string, LightRegion> getAvailableRegions();

    void setStatus(Color color);

    void colorRegion(std::string regionName, Color color);
    void colorRegionSection(std::string regionName, uint8_t section, Color color);
    void colorLEDs(uint8_t channel, uint16_t led, uint16_t count, Color color);
    void render(bool all = false, int8_t channel = -1);

    Color colorWheel(uint8_t pos);
    Color randomColor();

    static void startCalibrateLight(void* params);
    static void startUpdateLight(void *params);
    static void startAdvertisingLight(void *params);

    void applyEffect(LightingParameters parameters);

    static std::map<Actions, std::string> headlightActions;
    static std::map<Actions, std::string> motionActions;
    static std::map<Actions, std::string> turnActions;
    static std::map<Actions, std::string> orientationActions;
};