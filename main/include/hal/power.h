#pragma once
#include <common.h>

#ifdef MANAGES_INTERNAL_POWER
#include "FreeRTOS.h"
#include <interfaces/lifecycle.h>
#include <interfaces/power-listener.h>
#include <interfaces/touch-listener.h>

#define BATTERY_NORMAL 20
#define BATTERY_LOW 5
#define BATTERY_CHARGED 95

#if defined(AMP_1_0_x)
  #include <hal/amp-1.0.0/amp-power.h>
#endif

class Power : public PowerProvider, public TouchListener {
  std::vector<TouchType> touches;
  AmpPower ampPower;

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
#endif