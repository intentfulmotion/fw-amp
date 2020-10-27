#include <services/vehicle-service.h>

VehicleService::VehicleService(Motion *motion, Power *power, NimBLEServer *server, RenderHost *host) {
  _motion = motion;
  _server = server;
  _power = power;
  _renderHost = host;
  
  calibrateXGQueue = xQueueCreate(2, sizeof(CalibrationState));
  calibrateMagQueue = xQueueCreate(2, sizeof(CalibrationState));
  lightsChangedQueue = xQueueCreate(4, sizeof(LightCommands));

  setupService();
}

void VehicleService::setupService() {
  auto service = _server->createService(vehicleServiceUUID);

  _controlCharacteristic = service->createCharacteristic(
    NimBLEUUID::fromString(vehicleControlCharacteristicUUID),
    NIMBLE_PROPERTY::READ |
    NIMBLE_PROPERTY::WRITE |
    NIMBLE_PROPERTY::WRITE_NR |
    NIMBLE_PROPERTY::NOTIFY);
  
  _controlCharacteristic->setCallbacks(this);

  // set initial control
  uint8_t initialControl[3];
  initialControl[0] = _motion->isMotionDetectionEnabled() ? 0x01 : 0x02;
  initialControl[1] = _motion->isTurnDetectionEnabled() ? 0x01 : 0x02;
  initialControl[2] = _motion->isOrientationDetectionEnabled() ? 0x01 : 0x02;
  _controlCharacteristic->setValue(&initialControl[0], sizeof(initialControl));

  _stateCharacteristic = service->createCharacteristic(
    BLEUUID::fromString(vehicleStateCharacteristicUUID),
    NIMBLE_PROPERTY::READ |
    NIMBLE_PROPERTY::NOTIFY);

  _lightCharacteristic = service->createCharacteristic(
    NimBLEUUID::fromString(vehicleLightsCharacteristicUUID),
    NIMBLE_PROPERTY::READ | 
    NIMBLE_PROPERTY::WRITE |
    NIMBLE_PROPERTY::WRITE_NR |
    NIMBLE_PROPERTY::NOTIFY);

  _lightCharacteristic->setCallbacks(this);

  _calibrationCharacteristic = service->createCharacteristic(
    NimBLEUUID::fromString(vehicleCalibrationCharacteristicUUID),
    NIMBLE_PROPERTY::READ |
    NIMBLE_PROPERTY::WRITE |
    NIMBLE_PROPERTY::WRITE_NR |
    NIMBLE_PROPERTY::NOTIFY);

  _calibrationCharacteristic->setCallbacks(this);

  _restartCharacteristic = service->createCharacteristic(
    NimBLEUUID::fromString(vehicleRestartCharactersticUUID),
    NIMBLE_PROPERTY::WRITE | 
    NIMBLE_PROPERTY::WRITE_NR);
  
  _restartCharacteristic->setCallbacks(this);

  service->start();
}

void VehicleService::onWrite(NimBLECharacteristic *characteristic) {
  auto uuid = characteristic->getUUID();
  std::string dataStr = characteristic->getValue();
  const char* data = dataStr.data();
  size_t len = dataStr.length();

  if (uuid.equals(_controlCharacteristic->getUUID())) {
    ESP_LOGD(VEHICLE_SERVICE_TAG,"vehicle control onwrite");    
    uint8_t changed[3];
    
    if (len >= 1 && data[0] != 0x00) {
      bool enableAutoMotion = data[0] == 0x01;
      ESP_LOGD(VEHICLE_SERVICE_TAG,"change auto motion detection: %s", enableAutoMotion ? "enabled" : "disabled");
      _motion->setMotionDetection(enableAutoMotion);
      changed[0] = data[0];
    }
    else changed[0] = 0x00;

    if (len >= 2 && data[1] != 0x00) {
      bool enableAutoTurn = data[1] == 0x01;
      ESP_LOGD(VEHICLE_SERVICE_TAG,"change auto turn detection: %s", enableAutoTurn ? "enabled" : "disabled");
      _motion->setTurnDetection(enableAutoTurn);
      changed[1] = data[1];
    }
    else changed[1] = 0x00;

    if (len >= 3 && data[2] != 0x00) {
      bool enableAutoOrientation = data[2] == 0x01;
      ESP_LOGD(VEHICLE_SERVICE_TAG,"change auto orientation detection: %s", enableAutoOrientation ? "enabled" : "disabled");
      _motion->setOrientationDetection(enableAutoOrientation);
      changed[2] = data[2];
    }
    else changed[2] = 0x00;

    // notify that we've changed control
    if (changed[0] + changed[1] + changed[2] > 0) {
      _controlCharacteristic->setValue(&changed[0], sizeof(changed));
      _controlCharacteristic->notify(true);
    }
  }
  else if (uuid.equals(_lightCharacteristic->getUUID())) {
    ESP_LOGD(VEHICLE_SERVICE_TAG,"vehicle lights onwrite");

    // motion lights
    if (len >= 1 && data[0] != 0x00) {
      Actions command = (Actions)(data[0]);
      ESP_LOGD(VEHICLE_SERVICE_TAG,"change motion lights: %d", command);
      _renderHost->setMotion(command);
    }

    // headlights
    if (len >= 2 && data[1] != 0x00) {
      Actions command = (Actions)(data[1]);
      ESP_LOGD(VEHICLE_SERVICE_TAG,"change headlights: %d", command);
      _renderHost->setHeadlight(command);
    }

    // turn lights
    if (len >= 3 && data[2] != 0x00) {
      Actions command = (Actions)(data[2]);
      ESP_LOGD(VEHICLE_SERVICE_TAG,"change turn lights: %d", command);
      _renderHost->setTurnLights(command);
    }

    // orientation lights
    if (len >= 4 && data[2] != 0x00) {
      Actions command = (Actions)(data[3]);
      ESP_LOGD(VEHICLE_SERVICE_TAG,"change orientation lights: %d", command);
      _renderHost->setOrientationLights(command);
    }
  }
  else if (uuid.equals(_restartCharacteristic->getUUID())) {
    ESP_LOGD(VEHICLE_SERVICE_TAG,"vehicle restart onwrite: %d", data[0]);
    _power->shutdown(true);
  }
  else if (uuid.equals(_calibrationCharacteristic->getUUID())) {
    ESP_LOGD(VEHICLE_SERVICE_TAG,"vehicle calibration onwrite");
    if (len >= 1) {
      xQueueSendToBack(_motion->calibrationRequestQueue, &data[0], 0);
    }
  }
}

void VehicleService::onVehicleStateChanged(VehicleState state) {
  // ESP_LOGD(VEHICLE_SERVICE_TAG,"broadcast vehicle state changed");
  uint8_t value[3];
  value[0] = state.acceleration;
  value[1] = state.turn;
  value[2] = state.orientation;
  
  _stateCharacteristic->setValue((uint8_t*)value, sizeof(state));
  _stateCharacteristic->notify();
}

void VehicleService::process() {
  if (uxQueueMessagesWaiting(calibrateXGQueue)) {
    CalibrationState state;
    xQueueReceive(calibrateXGQueue, &state, 0);

    state == CalibrationState::Started ? onCalibrateXGStarted() : onCalibrateXGEnded();
  }
  
  if (uxQueueMessagesWaiting(calibrateMagQueue)) {
    CalibrationState state;
    xQueueReceive(calibrateMagQueue, &state, 0);

    state == CalibrationState::Started ? onCalibrateMagStarted() : onCalibrateMagEnded();
  }

  if (uxQueueMessagesWaiting(lightsChangedQueue)) {
    LightCommands commands;
    xQueueReceive(lightsChangedQueue, &commands, 0);

    onLightsChanged(commands);
  }
}

void VehicleService::onLightsChanged(LightCommands commands) {
  uint8_t payload[4];
  payload[0] = commands.motionCommand;
  payload[1] = commands.headlightCommand;
  payload[2] = commands.turnCommand;
  payload[3] = commands.orientationCommand;

  _lightCharacteristic->setValue(&payload[0], sizeof(payload));
  _lightCharacteristic->notify(true);
}

void VehicleService::onCalibrateXGStarted() {
  auto value = _calibrationCharacteristic->getValue();
  auto data = value.data();
  
  uint8_t payload[2];
  payload[0] = data[0];
  payload[1] = 0x01;

  _calibrationCharacteristic->setValue(&payload[0], sizeof(payload));
  _calibrationCharacteristic->notify();
}

void VehicleService::onCalibrateXGEnded() {
  auto value = _calibrationCharacteristic->getValue();
  auto data = value.data();
  
  uint8_t payload[2];
  payload[0] = data[0];
  payload[1] = 0x02;

  _calibrationCharacteristic->setValue(&payload[0], sizeof(payload));
  _calibrationCharacteristic->notify();
}

void VehicleService::onCalibrateMagStarted() {
  auto value = _calibrationCharacteristic->getValue();
  auto data = value.data();
  
  uint8_t payload[2];
  payload[0] = data[0];
  payload[1] = 0x03;

  _calibrationCharacteristic->setValue(&payload[0], sizeof(payload));
  _calibrationCharacteristic->notify();
}

void VehicleService::onCalibrateMagEnded() {
  auto value = _calibrationCharacteristic->getValue(); 
  auto data = value.data();
  
  uint8_t payload[2];
  payload[0] = data[0];
  payload[1] = 0x04;

  _calibrationCharacteristic->setValue(&payload[0], sizeof(payload));
  _calibrationCharacteristic->notify();
}