#include <app.h>
#include <models/light.h>

Amp* App::amp = nullptr;

App::App(Amp *instance) {
  amp = instance;
  amp->config.addConfigListener(this);

  configUpdatedQueue = xQueueCreate(1, sizeof(bool));
  // lightModeQueue = xQueueCreate(1, sizeof(LightMode));
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

  // notify lights changed
  // if (renderer != nullptr)
    // notifyLightsChanged(renderer->getBrakeCommand(), renderer->getTurnLightCommand(), renderer->getHeadlightCommand());
#endif

  // listen to motion changes
  amp->motion.addMotionListener(this);
}

void App::onPowerDown() {
  ESP_LOGD(APP_TAG,"App power down");

  // TODO: remove all lighting effects
  // if (renderHostHandle != NULL) {
  //   vTaskDelete(renderHostHandle);
  //   renderHostHandle = NULL;
  // }

  // if (renderer != NULL)
  //   renderer->shutdown();
}

void App::onConfigUpdated() {
  config = &Config::ampConfig;

  // TODO: setup lights somehow

  // reset motion detection
  amp->motion.resetMotionDetection();
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

  // if (renderer != NULL)
  //  renderer->process();
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
  LightCommand command;

  switch (state) {
    case Orientation::TopSideUp:
      command = LightCommand::LightsOff;
      break;
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
      ESP_LOGI(APP_TAG, "Applying effect %d to %s", effect.effect, effect.region.c_str());
      amp->lights->applyEffect(effect);
    }
  }

  _headlightCommand = command;

  // if (renderer != NULL && renderer->headlightQueue != NULL)
  //   xQueueSend(renderer->headlightQueue, &command, 0);

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
      ESP_LOGI(APP_TAG, "Applying effect %d to %s", effect.effect, effect.region.c_str());
      amp->lights->applyEffect(effect);
    }
  }
  // if (renderer != NULL && renderer->brakelightQueue != NULL)
  //   xQueueSend(renderer->brakelightQueue, &command, 0);

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
      ESP_LOGI(APP_TAG, "Applying effect %d to %s", effect.effect, effect.region.c_str());
      amp->lights->applyEffect(effect);
    }
  }

  _turnCommand = command;
  // if (renderer != NULL && renderer->turnlightQueue != NULL)
  //   xQueueSend(renderer->turnlightQueue, &command, 0);

  // update listeners
  notifyLightsChanged(NoCommand, command, NoCommand);
}

void App::notifyLightsChanged(LightCommand brakeCommand, LightCommand turnCommand, LightCommand headlightCommand) {
  LightCommands commands;
  commands.brakeCommand = brakeCommand;
  commands.turnCommand = turnCommand;
  commands.headlightCommand = headlightCommand;

  for (auto listener : renderListeners) {
    if (listener->lightsChangedQueue != NULL)
      xQueueSend(listener->lightsChangedQueue, &commands, 0);
  }
}