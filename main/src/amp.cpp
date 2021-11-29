#include <amp.h>
#include <app.h>

Lights *Amp::lights = Lights::instance();
Updater *Amp::updater = Updater::instance();
Config Amp::config;

#ifdef MANAGES_INTERNAL_POWER
Power *Amp::power = Power::instance();
#endif

#ifdef HAS_INTERNAL_IMU
MotionProvider *Amp::motion = Motion::instance();
#endif

#ifdef HAS_INPUT_BUTTON
Buttons Amp::buttons;
#endif

#ifdef HAS_VESC_CAN
VescCan *Amp::can = VescCan::instance();
MotionProvider *Amp::motion = VescCan::instance();
PowerProvider *Amp::power = VescCan::instance();
#endif

#ifdef BLE_ENABLED
BluetoothLE *Amp::ble = BluetoothLE::instance();
#endif

void Amp::init()
{
  // listen to power changes
  power->addPowerLevelListener(motion);
  power->addPowerLevelListener(lights);

  // listen to lifecycle changes
#ifdef HAS_INPUT_BUTTON
  power->addLifecycleListener(&buttons);
#endif

  power->addLifecycleListener(motion);
  power->addLifecycleListener(lights);

#ifdef BLE_ENABLED
  power->addLifecycleListener(ble);
#endif

  power->addLifecycleListener(&config);

  // listen to config changes
  config.addConfigListener(motion);
  config.addConfigListener(lights);

  // listen to touch changes
#ifdef HAS_INPUT_BUTTON
  buttons.addTouchListener(lights);
  buttons.addTouchListener(power);

#ifdef BLE_ENABLED
  // listen to touches to start advertising
  buttons.addTouchListener(ble);
#endif
#endif

#ifdef BLE_ENABLED
  // listen to advertising changes
  ble->addAdvertisingListener(lights);
#endif

  // listen to ota update status changes
  updater->addUpdateListener(lights);

  // setup app
  app = new App(this);
  power->addLifecycleListener(app);
  power->addPowerLevelListener(app);

  // setup the board
  power->startup();
}

void Amp::process()
{
  // todo: move this into the app host
  // when we're powering down, we need to halt processing
#ifdef MANAGES_INTERNAL_POWER
  PowerProvider::powerDown.wait("power");
  power->process();
#endif

  if (app != nullptr)
    app->process();
}