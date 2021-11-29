#pragma once

#include <interfaces/motion-provider.h>
#include <interfaces/power-provider.h>
#include <interfaces/application.h>

#ifdef MANAGES_INTERNAL_POWER
#include <hal/power.h>
#endif

#include <hal/lights.h>
#include <hal/config.h>
#include <hal/updater.h>

#ifdef HAS_VESC_CAN
#include <hal/vesc-can.h>
#endif

#ifdef HAS_BUTTON
#include <hal/buttons.h>
#endif

#ifdef HAS_INTERNAL_IMU
#include <hal/internal-motion.h>
#endif

#ifdef BLE_ENABLED
#include <hal/ble.h>
#endif

#if defined(AMP_1_0_x)
#include <hal/amp-1.0.0/amp-storage.h>
#endif

class Amp
{
public:
  static PowerProvider *power;
  static Lights *lights;
  static Updater *updater;
  static MotionProvider *motion;

#ifdef HAS_BUTTON
  static Buttons buttons;
#endif

  static Config config;

#ifdef HAS_VESC_CAN
  static VescCan *can;
#endif

#ifdef BLE_ENABLED
  static BluetoothLE *ble;
#endif

  Application *app = nullptr;

  static Amp *instance()
  {
    static Amp amp;
    return &amp;
  }

  void init();
  void process();
};