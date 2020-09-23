#include <renderers/running.h>

RunningRenderer::RunningRenderer(Lights *lights, AmpConfig *config) : Renderer(lights) {
  _config = config;

  headlightQueue = xQueueCreate(2, sizeof(LightCommand));
  brakelightQueue = xQueueCreate(2, sizeof(LightCommand));
  turnlightQueue = xQueueCreate(2, sizeof(LightCommand));

  // set the turn lights and auto re-apply headlights + brakes
  setTurnLights(LightCommand::LightsReset);
  ESP_LOGD(RUNNING_TAG,"Started running renderer");
}

void RunningRenderer::shutdown() {
  ESP_LOGD(RUNNING_TAG,"shutting down running renderer");
  brakeInit.give();
  turnInit.give();

  if (brakeHandle != nullptr) {
    ESP_LOGD(RUNNING_TAG,"Renderer: deleting existing brake task");
    vTaskDelete(brakeHandle);
    brakeHandle = nullptr;
  }

  if (turnHandle != nullptr) {
    ESP_LOGD(RUNNING_TAG,"Renderer: deleting existing turn task");
    vTaskDelete(turnHandle);
    turnHandle = nullptr;
  }

  this->~RunningRenderer();
}

void RunningRenderer::setBrakes(LightCommand command) {
  brakeInit.wait("brakes");

  // end existing brake task
  if (brakeHandle != nullptr || (command != LightsReset && _brakelightCommand == LightsBrake)) {
    ESP_LOGD(RUNNING_TAG,"Renderer: deleting existing brake task");
    vTaskDelete(brakeHandle);
    brakeHandle = nullptr;
  }

  switch (command) {
    // start a new brake flash process
    case LightCommand::LightsBrake:
      ESP_LOGD(RUNNING_TAG,"creating brake process");        
      brakeInit.take("brakes");
      xTaskCreatePinnedToCore(brakeFunction, "brake-light", 2048, this, 5, &brakeHandle, 1);
      break;

    // turn brake lights off
    case LightCommand::LightsOff:
      _lights->colorRegion("brake", lightOff);
      break;

    case LightCommand::LightsReset:
      if (_brakelightCommand != LightsBrake) {
        setBrakes(_brakelightCommand);
      }
      break;

    // set brake lights to running mode
    case LightCommand::LightsRunning:
    default:
      _lights->colorRegion("brake", runningBrake);
      break;
  }

  if (command != LightCommand::LightsReset)
    _brakelightCommand = command;
}

void RunningRenderer::setTurnLights(LightCommand command) {
  turnInit.wait("turn");

  // end existing turn light task
  bool isTurnCommand = _turnlightCommand >= LightsTurnLeft && _turnlightCommand <= LightsTurnHazard;
  if (turnHandle != nullptr || (command != LightsReset && isTurnCommand)) {
    ESP_LOGD(RUNNING_TAG,"Renderer: deleting existing turn task");
    vTaskDelete(turnHandle);
    turnHandle = nullptr;
  }
  
  switch (command) {
    case LightCommand::LightsTurnLeft:
    case LightCommand::LightsTurnRight:
    case LightCommand::LightsTurnHazard:
      turnInit.take("turn");
      xTaskCreatePinnedToCore(turnFunction, "turn-light", 4096, this, 5, &turnHandle, 1);
      break;
    case LightCommand::LightsReset:
      if (!isTurnCommand)
        setTurnLights(_turnlightCommand);

      setBrakes(LightsReset);
      setHeadlight(LightsReset);
      break;
    default:
      _lights->colorRegion("left", lightOff);
      _lights->colorRegion("right", lightOff);

      setBrakes(LightsReset);
      setHeadlight(LightsReset);
      break;
  }

  if (command != LightCommand::LightsReset)
    _turnlightCommand = command;
}

void RunningRenderer::setHeadlight(LightCommand command) {
  switch (command) {
    case LightCommand::LightsRunning:
      _lights->colorRegion("headlight", runningHeadlight);
      break;
    case LightCommand::LightsBright:
      _lights->colorRegion("headlight", brightHeadlight);
      break;
    case LightCommand::LightsReset:
      setHeadlight(_headlightCommand);
      break;
    case LightCommand::LightsOff:
    default:
      _lights->colorRegion("headlight", lightOff);
      break;
  }

  if (command != LightCommand::LightsReset)
    _headlightCommand = command;
}

void RunningRenderer::brakeFunction(void *args) {
  ESP_LOGD(RUNNING_TAG,"Renderer: started brake function");
  RunningRenderer *renderer = (RunningRenderer*)args;
  renderer->brakeInit.give();

  Lights *lights = renderer->_lights;
  AmpConfig *config = renderer->_config;

  lights->colorRegion("brake", renderer->brightBrake);
  lights->render();

  bool high = true;
  for (;;) {
    if (high)
      lights->colorRegion("brake", lightOff);
    else
      lights->colorRegion("brake", renderer->brightBrake);
    
    lights->render();
    
    high = !high;
    delay(config->prefs.brakeFlashRate);
  }

  ESP_LOGD(RUNNING_TAG,"Deleting brake task from inside brake task");
  vTaskDelete(NULL);
}

void RunningRenderer::turnFunction(void *args) {
  RunningRenderer *renderer = (RunningRenderer*)args;
  renderer->turnInit.give();

  Lights *lights = renderer->_lights;
  AmpConfig *config = renderer->_config;
  LightRegion region;
  switch (renderer->_turnlightCommand) {
    case LightCommand::LightsTurnLeft:
      region = lights->getLightRegion("left");
      break;
    case LightCommand::LightsTurnRight:
      region = lights->getLightRegion("right");
      break;
    case LightCommand::LightsTurnHazard:
      region.name = "hazard";
      auto left = lights->getLightRegion("left");
      auto right = lights->getLightRegion("right");

      region.sections.insert(region.sections.end(), left.sections.begin(), left.sections.end());
      region.sections.insert(region.sections.end(), right.sections.begin(), right.sections.end());
      break;
  }

  bool high = true;
  for (;;) {
    Color firstHalf = high ? renderer->turn : lightOff;
    Color secondHalf = high ? lightOff : renderer->turn;

    // reapply base lights for cases where indicators are the same as brake / headlights
    renderer->setBrakes(LightCommand::LightsReset);
    renderer->setHeadlight(LightCommand::LightsReset);

    for (auto section : region.sections) {
      bool remainder = (section.end - section.start) % 2 != 0;
      int half = (section.end - section.start) / 2;

      if (half > 1) {
        lights->colorLEDs(section.channel, section.start, remainder ? half : half - 1, firstHalf);
        lights->colorLEDs(section.channel, remainder ? half : half - 1, section.end, secondHalf);
      }
      else
        lights->colorLEDs(section.channel, section.start, section.end, firstHalf);

      lights->render();
    }

    high = !high;
    delay(config->prefs.turnFlashRate);
  }

  vTaskDelete(NULL);
}

void RunningRenderer::process() {
  if (uxQueueMessagesWaiting(headlightQueue)) {
    LightCommand command;

    if (xQueueReceive(headlightQueue, &command, 0))
      setHeadlight(command);
  }

  if (uxQueueMessagesWaiting(brakelightQueue)) {
    LightCommand command;

    if (xQueueReceive(brakelightQueue, &command, 0))
      setBrakes(command);
  }

  if (uxQueueMessagesWaiting(turnlightQueue)) {
    LightCommand command;
    
    if (xQueueReceive(turnlightQueue, &command, 0))
      setTurnLights(command);
  }
}