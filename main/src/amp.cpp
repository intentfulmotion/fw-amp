#include <amp.h>
#include <app.h>

Power* Amp::power = Power::instance();
Lights* Amp::lights = Lights::instance();
Updater* Amp::updater = Updater::instance();
Motion Amp::motion;
Buttons Amp::buttons;
Config Amp::config;

#ifdef BLE_ENABLED
BluetoothLE Amp::ble;
#endif

void Amp::init() {
  // listen to power changes
  power->addPowerLevelListener(&motion);
  power->addPowerLevelListener(lights);

  // listen to lifecycle changes
  power->addLifecycleListener(&buttons);
  power->addLifecycleListener(&motion);
  power->addLifecycleListener(lights);

#ifdef BLE_ENABLED
  power->addLifecycleListener(&ble);
#endif

  power->addLifecycleListener(&config);

  // listen to config changes
  config.addConfigListener(&motion);
  config.addConfigListener(lights);

#ifdef BLE_ENABLED
  config.addConfigListener(&ble);
#endif

  // listen to touch changes
  buttons.addTouchListener(lights);
  buttons.addTouchListener(power);

#ifdef BLE_ENABLED
  // listen to touches to start advertising
  buttons.addTouchListener(&ble);

  // listen to advertising changes
  ble.addAdvertisingListener(lights);
#endif

  // listen to ota update status changes
  updater->addUpdateListener(lights);
  
  // setup app
  app = new App(this);
  power->addLifecycleListener(app);

  // setup the board
  power->onPowerUp();
}

void Amp::process() {
  // todo: move this into the app host
  // when we're powering down, we need to halt processing
  Power::powerDown.wait("power");

  power->process();
  lights->process();

  if (app != nullptr)
    app->process();
}