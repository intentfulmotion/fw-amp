#include <app.h>
#include <models/light.h>

Amp* App::amp = nullptr;

App::App(Amp *instance) {
  amp = instance;
  amp->config.addConfigListener(this);

  configUpdatedQueue = xQueueCreate(1, sizeof(bool));
  vehicleQueue = xQueueCreate(5, sizeof(VehicleState));
}

void App::onPowerUp() { 
#ifdef BLE_ENABLED
  BluetoothLE::bleReady.wait();

  // workaround for weird bug where the first initialized service is duplicated / empty
  auto dummyService = amp->ble->server->createService(NimBLEUUID((uint16_t)0x183B));
  dummyService->start();
  
  // startup bluetooth services
  deviceInfoService = new DeviceInfoService(amp->ble->server);
  batteryService = new BatteryService(amp->ble->server);
  vehicleService = new VehicleService(&(amp->motion), amp->power, amp->ble->server, this);
  configService = new ConfigService(&(amp->config), amp->ble->server);
  updateService = new UpdateService(amp->updater, amp->ble->server);

  // listen to power updates
  amp->power->addPowerLevelListener(batteryService);

  // listen to render host changes for the vehicle service
  addRenderListener(vehicleService);

  // startup advertising
  amp->ble->startAdvertising();
#endif

  // listen to motion changes
  amp->motion.addMotionListener(this);
}

void App::onPowerDown() {
  ESP_LOGD(APP_TAG,"App power down");
}

void App::onConfigUpdated() {
  if (amp->config.isValid())
    config = &Config::ampConfig;  

  // reset motion detection
  amp->motion.resetMotionDetection();
  setBrakes(LightCommand::LightsReset);
  setTurnLights(LightCommand::LightsReset);
  setHeadlight(LightCommand::LightsReset);
}

void App::process() {
  bool valid;

  if (uxQueueMessagesWaiting(configUpdatedQueue)) {
    if (xQueueReceive(configUpdatedQueue, &valid, 0) && valid)
      onConfigUpdated();
  }

  VehicleState state;
  bool newVehicleState = false;

  // consume all new vehicle states from queue
  while (uxQueueMessagesWaiting(vehicleQueue))
    newVehicleState = xQueueReceive(vehicleQueue, &state, 0);

  if (newVehicleState) {
    if (vehicleState.acceleration != state.acceleration)
      onAccelerationStateChanged(state.acceleration);
    
    if (vehicleState.turn != state.turn)
      onTurnStateChanged(state.turn);

    if (vehicleState.orientation != state.orientation)
      onOrientationChanged(state.orientation);

  #ifdef BLE_ENABLED
    if (vehicleService != nullptr)
      vehicleService->onVehicleStateChanged(state);
  #endif
    
    vehicleState = state;
  }

#ifdef BLE_ENABLED
  vehicleService->process();
  batteryService->process();
  updateService->process();
#endif
}

void App::onAccelerationStateChanged(AccelerationState state) {
  ESP_LOGD(APP_TAG, "on acceleration state changed");
  LightCommand command;

  switch (state) {
    case AccelerationState::Braking:
      command = LightCommand::LightsBrakeActive;
      break;
    default:
    case AccelerationState::Neutral:
      command = LightCommand::LightsBrakeNormal;
      break;
  }

  setBrakes(command);
}

void App::onTurnStateChanged(TurnState state) {
  ESP_LOGD(APP_TAG, "on turn state changed");
  LightCommand command;

  switch (state) {
    case TurnState::Left:
      command = LightCommand::LightsTurnLeft;
      break;
    case TurnState::Right:
      command = LightCommand::LightsTurnRight;
      break;
    case TurnState::Hazard:
      command = LightCommand::LightsTurnHazard;
      break;
    case TurnState::Center:
    default:
      command = LightCommand::LightsTurnCenter;
      break;
  }

  setTurnLights(command);
}

void App::onOrientationChanged(Orientation state) {
  ESP_LOGD(APP_TAG, "on orientation state changed");
  LightCommand command;

  switch (state) {
    case Orientation::TopSideUp:
      command = LightCommand::LightsOff;
      break;
    case Orientation::FrontSideUp:
    case Orientation::BackSideUp:
      setTurnLights(LightCommand::LightsTurnCenter);
      setBrakes(LightCommand::LightsBrakeNormal);
      setHeadlight(LightCommand::LightsHeadlightNormal);
      return;
    default:
      command = LightCommand::LightsReset;
      break;
  }

  setTurnLights(command);
  setBrakes(command);
  setHeadlight(command);
}

void App::setHeadlight(LightCommand command) {
  auto actions = config->actions;
  if (command == LightCommand::LightsReset)
    command = _headlightCommand;

  std::string actionName = Lights::headlightActions[command];
  ESP_LOGI(APP_TAG, "Setting headlight - Command: %d, Action name: %s", command, actionName.c_str());

  if (actions.find(actionName) != actions.end()) {
    for (auto effect : *actions[actionName]) {
      ESP_LOGD(APP_TAG, "Applying effect %d to %s", effect.effect, effect.region.c_str());
      amp->lights->applyEffect(effect);
    }
  }

  _headlightCommand = command;

  // // update listeners
  notifyLightsChanged(NoCommand, NoCommand, command);
}

void App::setBrakes(LightCommand command) {
  auto actions = config->actions;
  if (command == LightCommand::LightsReset)
    command = _brakeCommand;

  std::string actionName = Lights::brakeActions[command];
  ESP_LOGI(APP_TAG, "Setting brakes - Command: %d, Action name: %s", command, actionName.c_str());

  if (actions.find(actionName) != actions.end()) {
    for (auto effect : *actions[actionName]) {
      ESP_LOGD(APP_TAG, "Applying effect %d to %s", effect.effect, effect.region.c_str());
      amp->lights->applyEffect(effect);
    }
  }

  _brakeCommand = command;

  // update listeners
  notifyLightsChanged(command, NoCommand, NoCommand);
}

void App::setTurnLights(LightCommand command) {
  auto actions = config->actions;
  if (command == LightCommand::LightsReset)
    command = _turnCommand;

  std::string actionName = Lights::turnActions[command];
  ESP_LOGI(APP_TAG, "Setting indicators - Command: %d, Action name: %s", command, actionName.c_str());
  
  if (actions.find(actionName) != actions.end()) {
    for (auto effect : *actions[actionName]) {
      ESP_LOGD(APP_TAG, "Applying effect %d to %s", effect.effect, effect.region.c_str());
      amp->lights->applyEffect(effect);
    }
  }

  _turnCommand = command;

  // update listeners
  notifyLightsChanged(NoCommand, command, NoCommand);
}

void App::notifyLightsChanged(LightCommand brakeCommand, LightCommand turnCommand, LightCommand headlightCommand) {
  LightCommands commands;
  commands.brakeCommand = brakeCommand == LightCommand::NoCommand || brakeCommand == LightCommand::LightsReset ? _brakeCommand : brakeCommand;
  commands.turnCommand = turnCommand == LightCommand::NoCommand || turnCommand == LightCommand::LightsReset ? _turnCommand : turnCommand;
  commands.headlightCommand = headlightCommand == LightCommand::NoCommand || headlightCommand == LightCommand::LightsReset ? _headlightCommand : headlightCommand;

  for (auto listener : renderListeners) {
    if (listener->lightsChangedQueue != NULL)
      xQueueSend(listener->lightsChangedQueue, &commands, 0);
  }
}