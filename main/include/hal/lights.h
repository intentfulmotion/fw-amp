#pragma once
#include <vector>

#include <common.h>
#include <interfaces/lifecycle.h>
#include <interfaces/power-listener.h>
#include <interfaces/touch-listener.h>
#include <interfaces/config-listener.h>
#include <interfaces/ble-listener.h>
#include <interfaces/calibration-listener.h>
#include <interfaces/update-listener.h>
#include <models/light.h>

#if defined(AMP_1_0_x)
  #include <hal/amp-1.0.0/amp-leds.h>
  #include <hal/amp-1.0.0/amp-storage.h>
#endif

#include <hal/config.h>

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

  public:
    Lights();
    std::map<int, LightController*> channelMap;
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

    void setStatus(ColorRGB color);

    void colorRegion(std::string region, ColorRGB color, ColorRGBW extendedColor = NULL);
    void colorRegionSection(std::string region, uint8_t section, ColorRGB color, ColorRGBW extendedColor = NULL);
    void colorLEDs(uint8_t channel, uint16_t led, uint16_t count, ColorRGB color, ColorRGBW extendedColor = NULL);
    void render();

    ColorRGB Wheel(uint8_t pos);
    ColorRGB randomColorRGB();

    static void startCalibrateLight(void* params);
    static void startUpdateLight(void *params);
    static void startAdvertisingLight(void *params);
};