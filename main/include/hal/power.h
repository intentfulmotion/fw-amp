#pragma once
#include "FreeRTOS.h"

#include <vector>
#include <common.h>
#include <interfaces/lifecycle.h>
#include <interfaces/power-listener.h>
#include <interfaces/touch-listener.h>

#define BATTERY_NORMAL 20
#define BATTERY_LOW 5
#define BATTERY_CHARGED 95

#if defined(AMP_1_0_x)
  #include <hal/amp-1.0.0/amp-power.h>
#endif

class Power : public LifecycleBase, public TouchListener {
  std::vector<PowerListener*> powerLevelListeners;
  std::vector<LifecycleBase*> lifecycleListeners;
  std::vector<TouchType> touches;
  PowerStatus status;
  AmpPower ampPower;
  bool restartNext = false;

  void notifyPowerListeners();
  PowerStatus calculatePowerStatus(bool batteryPresent, bool charging, bool done, uint8_t batteryLevel);

  public:
    Power();
    static Power* instance() { static Power power; return &power; }
    void onPowerUp();
    void onPowerDown();
    void process();

    void onTouchEvent(std::vector<TouchType> *touches);
    void shutdown(bool restart = false);

    void addPowerLevelListener(PowerListener *listener);
    void addLifecycleListener(LifecycleBase *listener);

    static FreeRTOS::Semaphore powerDown;
};