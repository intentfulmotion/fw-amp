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
    if (len >= 1 && data[0] != 0x00) {
      bool enableAutoBrakes = data[0] == 0x01;
      ESP_LOGD(VEHICLE_SERVICE_TAG,"change auto brake detection: %s", enableAutoBrakes ? "enabled" : "disabled");
      _motion->setBrakeDetection(enableAutoBrakes);
    }
    if (len >= 2 && data[1] != 0x00) {
      bool enableAutoTurn = data[1] == 0x01;
      ESP_LOGD(VEHICLE_SERVICE_TAG,"change auto turn detection: %s", enableAutoTurn ? "enabled" : "disabled");
      _motion->setTurnDetection(enableAutoTurn);
    }
    if (len >= 3 && data[2] != 0x00) {
      bool enableAutoOrientation = data[2] == 0x01;
      ESP_LOGD(VEHICLE_SERVICE_TAG,"change auto orientation detection: %s", enableAutoOrientation ? "enabled" : "disabled");
      _motion->setOrientationDetection(enableAutoOrientation);
    }      
  }
  else if (uuid.equals(_lightCharacteristic->getUUID())) {
    ESP_LOGD(VEHICLE_SERVICE_TAG,"vehicle lights onwrite");

    // brake lights
    if (len >= 1 && data[0] != 0x00) {
      LightCommand command = (LightCommand)(data[0]);
      ESP_LOGD(VEHICLE_SERVICE_TAG,"change brakes: %d", command);
      _renderHost->setBrakes(command);
    }

    // headlights
    if (len >= 2 && data[1] != 0x00) {
      LightCommand command = (LightCommand)(data[1]);
      ESP_LOGD(VEHICLE_SERVICE_TAG,"change headlights: %d", command);
      _renderHost->setHeadlight(command);
    }

    // turn lights
    if (len >= 3 && data[2] != 0x00) {
      LightCommand command = (LightCommand)(data[2]);
      ESP_LOGD(VEHICLE_SERVICE_TAG,"change turn lights: %d", command);
      _renderHost->setTurnLights(command);
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
  // union {
  //   LightCommands commands = commands;
  //   uint8_t bytes[4];
  // };
  
  // _lightCharacteristic->setValue((uint8_t*) &bytes, sizeof(uint8_t) * 4);
  _lightCharacteristic->setValue<LightCommands>(commands);
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