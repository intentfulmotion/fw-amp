#pragma once
#include "common.h"
#include <vector>
#include "hal/config.h"
#include <interfaces/lifecycle.h>
#include <interfaces/power-listener.h>

#define BATTERY_NORMAL 20
#define BATTERY_LOW 5
#define BATTERY_CHARGED 95

static const char* POWER_TAG = "power";

class PowerProvider {
  protected:
    std::vector<PowerListener*> powerLevelListeners;
    std::vector<LifecycleBase*> lifecycleListeners;
    PowerStatus powerStatus;
    bool restartNext = false;

    void notifyPowerListeners();
    PowerStatus calculatePowerStatus(bool batteryPresent, bool charging, bool done, uint8_t batteryLevel);

  public:
    virtual void shutdown(bool restart = false) = 0;

    void addPowerLevelListener(PowerListener *listener);
    void addLifecycleListener(LifecycleBase *listener);

    static FreeRTOS::Semaphore powerDown;

    virtual void startup() = 0;
    virtual void process() = 0;
};