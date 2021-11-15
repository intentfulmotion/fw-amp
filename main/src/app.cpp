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
  vehicleService = new VehicleService(amp->motion, amp->power, amp->ble->server, this);
  configService = new ConfigService(&(amp->config), amp->ble->server);
  updateService = new UpdateService(amp->updater, amp->ble->server);
  
#ifdef HAS_VESC_CAN
  vescService = new VescService(amp->can, amp->ble->server);
#endif

  // listen to power updates
  amp->power->addPowerLevelListener(batteryService);

  // listen to render host changes for the vehicle service
  addRenderListener(vehicleService);

  // startup advertising
  amp->ble->startAdvertising();
#endif

  // listen to motion changes
  amp->motion->addMotionListener(this);
}

void App::onPowerDown() {
  ESP_LOGD(APP_TAG,"App power down");
}

void App::onConfigUpdated() {
  if (amp->config.isValid())
    config = &Config::ampConfig;  

  // reset motion detection
  amp->motion->resetMotionDetection();
  setMotion(Actions::LightsReset);
  setTurnLights(Actions::LightsReset);
  setHeadlight(Actions::LightsReset);
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

    if (vehicleState.direction != state.direction)
      onDirectionChanged();

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
  Actions command;

  switch (state) {
    case AccelerationState::Braking:
      command = Actions::LightsMotionBrakes;
      break;
    case AccelerationState::Accelerating:
      command = Actions::LightsMotionAcceleration;
      break;
    default:
    case AccelerationState::Neutral:
      command = Actions::LightsMotionNeutral;
      break;
  }

  setMotion(command);
}

void App::onTurnStateChanged(TurnState state) {
  ESP_LOGD(APP_TAG, "on turn state changed");
  Actions command;

  switch (state) {
    case TurnState::Left:
      command = Actions::LightsTurnLeft;
      break;
    case TurnState::Right:
      command = Actions::LightsTurnRight;
      break;
    case TurnState::Hazard:
      command = Actions::LightsTurnHazard;
      break;
    case TurnState::Center:
    default:
      command = Actions::LightsTurnCenter;
      break;
  }

  setTurnLights(command);
}

void App::onOrientationChanged(Orientation state) {
  ESP_LOGD(APP_TAG, "on orientation state changed");
  Actions command;

  switch (state) {
    case Orientation::UnknownSideUp:
      command = Actions::LightsOrientationUnknown;
      break;
    case Orientation::TopSideUp:
      command = Actions::LightsOrientationTop;
      break;
    case Orientation::BottomSideUp:
      command = Actions::LightsOrientationBottom;
      break;
    case Orientation::LeftSideUp:
      command = Actions::LightsOrientationLeft;
      break;
    case Orientation::RightSideUp:
      command = Actions::LightsOrientationRight;
      break;
    case Orientation::FrontSideUp:
      command = Actions::LightsOrientationFront;
      break;
    case Orientation::BackSideUp:
      command = Actions::LightsOrientationBack;
      break;
    default:
      command = Actions::LightsReset;
      break;
  }

  setOrientationLights(command);
}

void App::onDirectionChanged() {
  setMotion(Actions::LightsReset);
  setTurnLights(Actions::LightsReset);
  setHeadlight(Actions::LightsReset);
}

void App::setHeadlight(Actions command) {
  auto actions = config->actions;
  if (command == Actions::LightsReset)
    command = _headlightCommand;

  std::string actionName = Lights::headlightActions[command];
  std::string actionNameWithDirection = actionName + (vehicleState.direction == Direction::Forward ? "-forward" : "-backward");
  bool useActionWithDirection = actions.find(actionNameWithDirection) != actions.end();
  std::string actionToUse = useActionWithDirection ? actionNameWithDirection : actionName;
  ESP_LOGI(APP_TAG, "Setting headlight - Command: %d, Action name: %s", command, actionToUse.c_str());

  if (useActionWithDirection || actions.find(actionName) != actions.end()) {
    for (auto effect : *actions[actionToUse]) {
      ESP_LOGD(APP_TAG, "Applying effect %d to %s", effect.effect, effect.region.c_str());
      amp->lights->applyEffect(effect);
    }
  }

  _headlightCommand = command;

  // // update listeners
  notifyLightsChanged(NoCommand, NoCommand, command, NoCommand);
}

void App::setMotion(Actions command) {
  auto actions = config->actions;
  if (command == Actions::LightsReset)
    command = _motionCommand;

  std::string actionName = Lights::motionActions[command];
  std::string actionNameWithDirection = actionName + (vehicleState.direction == Direction::Forward ? "-forward" : "-backward");
  bool useActionWithDirection = actions.find(actionNameWithDirection) != actions.end();
  std::string actionToUse = useActionWithDirection ? actionNameWithDirection : actionName;
  ESP_LOGI(APP_TAG, "Setting motion - Command: %d, Action name: %s", command, actionToUse.c_str());

  if (useActionWithDirection || actions.find(actionName) != actions.end()) {
    for (auto effect : *actions[actionToUse]) {
      ESP_LOGD(APP_TAG, "Applying effect %d to %s", effect.effect, effect.region.c_str());
      amp->lights->applyEffect(effect);
    }
  }

  _motionCommand = command;

  // update listeners
  notifyLightsChanged(command, NoCommand, NoCommand, NoCommand);
}

void App::setTurnLights(Actions command) {
  auto actions = config->actions;
  if (command == Actions::LightsReset)
    command = _turnCommand;

  std::string actionName = Lights::turnActions[command];
  std::string actionNameWithDirection = actionName + (vehicleState.direction == Direction::Forward ? "-forward" : "-backward");
  bool useActionWithDirection = actions.find(actionNameWithDirection) != actions.end();
  std::string actionToUse = useActionWithDirection ? actionNameWithDirection : actionName;
  ESP_LOGI(APP_TAG, "Setting indicators - Command: %d, Action name: %s", command, actionToUse.c_str());
  
  if (useActionWithDirection || actions.find(actionName) != actions.end()) {
    for (auto effect : *actions[actionToUse]) {
      ESP_LOGD(APP_TAG, "Applying effect %d to %s", effect.effect, effect.region.c_str());
      amp->lights->applyEffect(effect);
    }
  }

  _turnCommand = command;

  // update listeners
  notifyLightsChanged(NoCommand, command, NoCommand, NoCommand);
}

void App::setOrientationLights(Actions command) {
  auto actions = config->actions;
  if (command == Actions::LightsReset)
    command = _orientationCommand;

  std::string actionName = Lights::orientationActions[command];
  ESP_LOGI(APP_TAG, "Setting orientation - Command: %d, Action name: %s", command, actionName.c_str());
  
  if (actions.find(actionName) != actions.end()) {
    for (auto effect : *actions[actionName]) {
      ESP_LOGD(APP_TAG, "Applying effect %d to %s", effect.effect, effect.region.c_str());
      amp->lights->applyEffect(effect);
    }
  }

  _orientationCommand = command;

  // update listeners
  notifyLightsChanged(NoCommand, NoCommand, NoCommand, command);
}

void App::notifyLightsChanged(Actions motionCommand, Actions turnCommand, Actions headlightCommand, Actions orientationCommand) {
  LightCommands commands;
  commands.motionCommand = motionCommand == Actions::NoCommand || motionCommand == Actions::LightsReset ? _motionCommand : motionCommand;
  commands.turnCommand = turnCommand == Actions::NoCommand || turnCommand == Actions::LightsReset ? _turnCommand : turnCommand;
  commands.headlightCommand = headlightCommand == Actions::NoCommand || headlightCommand == Actions::LightsReset ? _headlightCommand : headlightCommand;
  commands.orientationCommand = orientationCommand == Actions::NoCommand || orientationCommand == Actions::LightsReset ? _orientationCommand : orientationCommand;

  for (auto listener : renderListeners) {
    if (listener->lightsChangedQueue != NULL)
      xQueueSend(listener->lightsChangedQueue, &commands, 0);
  }
}