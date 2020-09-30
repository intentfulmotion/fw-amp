#pragma once

#include <hal/power.h>
#include <hal/buttons.h>
#include <hal/lights.h>
#include <hal/motion.h>
#include <hal/config.h>
#include <hal/updater.h>

#ifdef BLE_ENABLED
  #include <hal/ble.h>
#endif

#if defined(AMP_1_0_x)
  #include <hal/amp-1.0.0/amp-storage.h>
#endif

class Amp {
  public:
    static Power *power;
    static Lights *lights;
    static Updater *updater;
    static Motion motion;
    static Buttons buttons;
    static Config config;

#ifdef BLE_ENABLED
    static BluetoothLE *ble;
#endif

    LifecycleBase *app = nullptr;

    static Amp* instance() { static Amp amp; return &amp; }

    void init();
    void process();
};