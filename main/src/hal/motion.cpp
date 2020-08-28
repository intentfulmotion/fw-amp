#include <hal/motion.h>

QueueHandle_t Motion::calibrationRequestQueue = NULL;
FreeRTOS::Semaphore Motion::holdInterface = FreeRTOS::Semaphore("spi");
AmpIMU Motion::ampIMU;

Motion::Motion() {
  configUpdatedQueue = xQueueCreate(1, sizeof(bool));
  calibrationRequestQueue = xQueueCreate(2, sizeof(uint8_t));
  powerStatusQueue = xQueueCreate(2, sizeof(PowerStatus));
}

void Motion::onPowerUp() {
  if (!ampIMU.init()) {
    imuState = IMU_Error;
    Log::error("Error starting motion");
  }
  else {
    Log::trace("Motion started");
    _turnZero = AmpStorage::getTurnZero();
    // load motion biases
    AmpStorage::getAccelBias(&accelBias);

    // start motion process
    xTaskCreatePinnedToCore(sampleTask, "motion", 4096, this, 1, &samplerHandle, 0);    
  }
}

void Motion::sampleTask(void *parameters) {
  Motion *motion = (Motion*)parameters;

  // reset last update
  motion->_lastUpdate = micros();

  VehicleState old = motion->_vehicleState;
  for (;;) {
    Power::powerDown.wait("power");
    motion->process();

    if (motion->_enabled && motion->imuState > IMUState::IMU_Disabled) {
      motion->sample();
      
      if (!motion->_calibrating) {
        if (motion->_autoOrientation)
          motion->detectOrientation();

        if (motion->_autoBrake)
          motion->detectBraking();

        if (motion->_autoTurn)
          motion->detectTurning();
      }
    }

    if (old != motion->_vehicleState) {
      Log::trace("vehicle state changed in motion");

      for (auto listener : motion->motionListeners)
        if (listener->vehicleQueue != NULL)
          xQueueSendToFront(listener->vehicleQueue, (void*) &motion->_vehicleState, 0);

      old = motion->_vehicleState;
    }

    delay(10);
  }

  vTaskDelete(NULL);
}

void Motion::onPowerDown() {
  Log::trace("Motion on power down");
  imuState = IMUState::IMU_Disabled;
  ampIMU.deinit();
}

void Motion::onPowerStatusChanged(PowerStatus status) {
  _powerStatus = status;
  updateMotionForPowerStatus(_powerStatus);
}

void Motion::updateMotionForPowerStatus(PowerStatus status) {
  if (imuState > IMUState::IMU_Error)
  {
    if (status.charging) {
      imuState = IMUState::IMU_Normal;
      _sampleRate = 50000;  // 50 kHz
    }
    else {
      switch (_powerStatus.level) {
        case PowerLevel::Low:
          imuState = IMUState::IMU_LowPower;
          Log::verbose("IMU: Low power mode");
          _sampleRate = 1500; // 1.5 kHz

#if defined(USE_MADGWICK_FILTER)
          filter = filter == nullptr ? new Madgwick() : filter;
#elif defined(USE_SIMPLE_AHRS_FILTER)
          filter = filter == nullptr ? new SimpleAHRS() : filter;
#endif
          break;
        case PowerLevel::Normal:
        case PowerLevel::Charged:
          imuState = IMUState::IMU_Normal;
          Log::verbose("IMU: Normal, high power mode");
          _sampleRate = 50000;  // 50 kHz

#if defined(USE_MADGWICK_FILTER)
          filter = filter == nullptr ? new Madgwick() : filter;
#elif defined(USE_SIMPLE_AHRS_FILTER)
          filter = filter == nullptr ? new SimpleAHRS() : filter;
#endif
          break;
        case PowerLevel::Critical:
        case PowerLevel::Unknown:
        default:
          imuState = IMUState::IMU_Disabled;
          Log::verbose("IMU: Disabled");
          _sampleRate = 0;

#if defined(USE_MADGWICK_FILTER) or defined(USE_SIMPLE_AHRS_FILTER)
          filter = nullptr;
#endif
          break;
      }
    }

    holdInterface.wait("spi");
    holdInterface.take("spi");
    Log::verbose("New IMU State: %d", imuState);
    ampIMU.setPowerMode(imuState);
    holdInterface.give();
  }
}

void Motion::process() {
  if (uxQueueMessagesWaiting(configUpdatedQueue)) {
    bool valid;
    xQueueReceive(configUpdatedQueue, &valid, 0);

    if (valid)
      onConfigUpdated();
  }

  if (uxQueueMessagesWaiting(powerStatusQueue)) {
    xQueueReceive(powerStatusQueue, &_powerStatus, 0);
    onPowerStatusChanged(_powerStatus);
  }

  if (uxQueueMessagesWaiting(calibrationRequestQueue)) {
    uint8_t request;
    xQueueReceive(calibrationRequestQueue, &request, 0);
    Log::trace("calibration request: %d", request);

    if (request == 0x01)
      calibrateXG();
    else if (request == 0x02)
      calibrateMag();
  }
}

void Motion::calibrateXG() {
  holdInterface.wait("spi");
  holdInterface.take("spi");

  _calibrating = true;
  CalibrationState state = CalibrationState::Started;

  for (auto listener : calibrationListeners)
    if (listener->calibrateXGQueue != NULL)
      xQueueSendToBack(listener->calibrateXGQueue, &state, 0);

  Vector3D biases[2];
  ampIMU.calibrateXG(&biases[0]);
  accelBias = biases[0];

  AmpStorage::saveAccelBias(&accelBias);
  printf("accel bias: %.3f, %.3f, %.3f\n", accelBias.x, accelBias.y, accelBias.z);

  state = CalibrationState::Ended;

  for (auto listener : calibrationListeners)
    if (listener->calibrateXGQueue != NULL)
      xQueueSendToBack(listener->calibrateXGQueue, &state, 0);
  
  _calibrating = false;

  holdInterface.give();
}

void Motion::calibrateMag() {
  holdInterface.wait("spi");
  holdInterface.take("spi");

  _calibrating = true;
  CalibrationState state = CalibrationState::Started;

  for (auto listener : calibrationListeners)
    if (listener->calibrateMagQueue != NULL)
      xQueueSendToBack(listener->calibrateMagQueue, &state, 0);

  ampIMU.calibrateMag(&magBias);
  state = CalibrationState::Ended;

  for (auto listener : calibrationListeners)
    if (listener->calibrateMagQueue != NULL)
      xQueueSendToBack(listener->calibrateMagQueue, &state, 0);

  _calibrating = false;

  holdInterface.give();
}

/*
  Updates gravity and linear acceleration vectors
*/
void Motion::calculateAccelerations(Vector3D raw) {
  // gravity calculation
  gravity.x = _alpha * gravity.x + (1 - _alpha) * raw[0];
  gravity.y = _alpha * gravity.y + (1 - _alpha) * raw[1];
  gravity.z = _alpha * gravity.z + (1 - _alpha) * raw[2];

  absoluteGravity.x = abs(gravity.x);
  absoluteGravity.y = abs(gravity.y);
  absoluteGravity.z = abs(gravity.z);

#if defined(LOG_MOTION_GRAVITY)
  Log::verbose("Gravity - X: %F Y: %F Z: %F", gravity.x, gravity.y, gravity.z);
#endif

  // linear acceleration
  linearAcceleration.x = raw[0] - gravity.x;
  linearAcceleration.y = raw[1] - gravity.y;
  linearAcceleration.z = raw[2] - gravity.z;

#if defined(LOG_MOTION_LINEAR_ACCELERATION)
  Log::verbose("$%.2f %.2f %.2f;", linearAcceleration.x, linearAcceleration.y, linearAcceleration.z);
#endif
}

void Motion::sample() {
  unsigned long current = micros();
  float diff = (current - _lastSample) / 1000000.0f;
  float frequency = 1.0f / diff;
  if (_sampleRate > 0 && frequency <= _sampleRate) {
    holdInterface.wait("spi");
    holdInterface.take("spi");
    ampIMU.process();

    // accelerometer
    rawAccel = ampIMU.getAccelData();
    rawAccel = rawAccel - accelBias;
#if defined(LOG_MOTION_RAW_ACCELERATION)
    // printf("Raw Accel - X: %.3f Y: %.3f Z: %.3f\n", rawAccel.x, rawAccel.y, rawAccel.z);
#endif
    calculateAccelerations(rawAccel);
    
    // gyro
    rawGyro = ampIMU.getGyroData();
    rawGyro = rawGyro - gyroBias;
#if defined(LOG_MOTION_RAW_GYRO)
    printf("Raw Gyro - X: %.3f Y: %.3f Z: %.3f\n", rawGyro.x, rawGyro.y, rawGyro.z);
#endif


    // mag
    rawMag = ampIMU.getMagData();
    rawMag = rawMag - magBias;
#if defined(LOG_MOTION_RAW_MAG)
    printf("Raw Mag - X: %.3f Y: %.3f Z: %.3f\n", rawMag.x, rawMag.y, rawMag.z);
#endif

    holdInterface.give();
    _lastSample = current;

    // update AHRS
    current = micros();
    _lastUpdate = current;

#if defined(USE_MADGWICK_FILTER)
    float diff = (current - _lastUpdate) / 1000000.0f;
    filter->update(
          rawAccel.x, rawAccel.y, rawAccel.z,
          rawGyro.x * PI / 180.0f, rawGyro.y * PI / 180.0f, rawGyro.z * PI / 180.0f,
          -rawMag.y, -rawMag.x, rawMag.z, diff);    
#elif defined(USE_SIMPLE_AHRS_FILTER)
    filter->update(rawAccel, rawMag);
#endif
    
    // filter->update(rawAccel, rawMag);

#if defined(USE_MADGWICK_FILTER) or defined(USE_SIMPLE_AHRS_FILTER)
    attitude = filter->getAHRS();

#if defined(LOG_MOTION_AHRS)
    printf("%lu - Orientation: %.3f %.3f %.3f\n", millis(), attitude.x, attitude.y, attitude.z);
#endif

    // compensate roll for orientation using the gravity vector
    if (_vehicleState.orientation != TopSideUp && _vehicleState.orientation != BottomSideUp)
      if (absoluteGravity.x > absoluteGravity.y)
        attitude.x += attitude.x * gravity.x;
      else
        attitude.x += attitude.x * gravity.y;

#if defined(LOG_MOTION_AHRS_COMPENSATED)
    printf("%lu - Orientation: %.3f %.3f %.3f\n", millis(), attitude.x, attitude.y, attitude.z);
#endif
#endif

    // printf("step:a - %.4f, %.4f, %.4f\tg - %.4f, %.4f, %.4f\tm - %.4f, %.4f, %.4f\n");

#if defined(LOG_SAMPLE_RATE)
    printf("Delta time: %.6f, frequency: %.2f Hz\n", diff, 1.0f / diff);
#endif
  }
}

void Motion::updateGravityFilter(float alpha) {
  _alpha = alpha;
}

void Motion::updateTurnCenter(float turnCenter) {
  _turnZero = turnCenter;
  AmpStorage::saveTurnZero(_turnZero);
}

void Motion::onConfigUpdated() {
  auto motion = Config::ampConfig.motion;

  // update motion detection
  if (!motion.autoMotion && !motion.autoOrientation && !motion.autoTurn)
    _enabled = false;
  else
    _enabled = true;

  setBrakeDetection(motion.autoMotion, motion.brakeAxis, motion.brakeThreshold);
  setTurnDetection(motion.autoTurn, motion.relativeTurnZero, motion.turnAxis, motion.turnThreshold);
  setOrientationDetection(motion.autoOrientation, motion.orientationAxis, motion.orientationUpMin, motion.orientationUpMax);

  resetMotionDetection();
}

void Motion::addMotionListener(MotionListener *listener) {
  motionListeners.push_back(listener);
}

void Motion::removeMotionListener(MotionListener *listener) {
  motionListeners.erase(std::remove(motionListeners.begin(), motionListeners.end(), listener));
}

void Motion::resetMotionDetection() {
  if (Config::ampConfig.motion.autoMotion)
    _vehicleState.acceleration = AccelerationState::Neutral;

  if (Config::ampConfig.motion.autoTurn)
    _vehicleState.turn = TurnState::Center;

  if (Config::ampConfig.motion.autoOrientation)
    _vehicleState.orientation = Orientation::TopSideUp;

  notifyMotionListeners();
}

void Motion::notifyMotionListeners() {
  // notify any listeners of the reset
  for (auto listener : motionListeners) {
    // vehicle state changed
    if (listener->vehicleQueue != NULL)
      xQueueSend(listener->vehicleQueue, &_vehicleState, 0);
  }
}

void Motion::setBrakeDetection(bool enabled, AccelerationAxis axis, float threshold) {
  _autoBrake = enabled;
  _brakeAxis = axis;
  _brakeThreshold = threshold;

  if (_autoBrake || _autoTurn || _autoOrientation)
    _enabled = true;
  else if (!_autoBrake && !_autoTurn && !_autoOrientation)
    _enabled = false;
}

void Motion::setTurnDetection(bool enabled, bool useRelativeTurnZero, AttitudeAxis axis, float threshold) {
  _autoTurn = enabled;
  _useRelativeTurnZero = useRelativeTurnZero;
  _turnAxis = axis;
  _turnThreshold = threshold;

  if (_autoBrake || _autoTurn || _autoOrientation)
    _enabled = true;
  else if (!_autoBrake && !_autoTurn && !_autoOrientation)
    _enabled = false;
}

void Motion::setOrientationDetection(bool enabled, AttitudeAxis axis, uint16_t min, uint16_t max) {
  _autoOrientation = enabled;
  _orientationAxis = axis;
  _orientationMin = min;
  _orientationMax = max;

  if (_autoBrake || _autoTurn || _autoOrientation)
    _enabled = true;
  else if (!_autoBrake && !_autoTurn && !_autoOrientation)
    _enabled = false;
}

void Motion::triggerVehicleState(VehicleState state, bool autoBrake, bool autoTurn, bool autoOrient) {
  _autoBrake = autoBrake;
  _autoTurn = autoTurn;
  _autoOrientation = autoOrient;

  _vehicleState = state;
  Log::verbose("vehicle state change. accel: %d", state.acceleration);
  notifyMotionListeners();
}

void Motion::triggerAccelerationState(AccelerationState state, bool autoBrake) {
  _vehicleState.acceleration = state;
  triggerVehicleState(_vehicleState, autoBrake, _autoTurn, _autoOrientation);
}

void Motion::triggerTurnState(TurnState state, bool autoTurn) {
  _vehicleState.turn = state;
  triggerVehicleState(_vehicleState, _autoBrake, autoTurn, _autoOrientation);
}

void Motion::triggerOrientationState(Orientation state, bool autoOrientation) {
  _vehicleState.orientation = state;
  triggerVehicleState(_vehicleState, _autoBrake, _autoTurn, autoOrientation);
}

bool Motion::detectBraking() {
  AccelerationState newAcceleration;
  unsigned long now = millis();
  auto debounce = _vehicleState.acceleration == AccelerationState::Braking ? BRAKE_ACTIVE_DEBOUNCE : BRAKE_DEBOUNCE;
  if (now - _lastBrakeUpdate > debounce) {
    float acceleration = getAccelerationFromAxis(_brakeAxis);

    switch (_vehicleState.acceleration) {
      case Braking:
        if (acceleration <= _brakeThreshold)
          newAcceleration = AccelerationState::Neutral;
        break;
      default:
        if (acceleration > _brakeThreshold)
          newAcceleration = AccelerationState::Braking;
    }

    if (acceleration >= _brakeThreshold)
      newAcceleration = AccelerationState::Braking;
    else
      newAcceleration = AccelerationState::Neutral;

    // Log::verbose("%.3f, %d", acceleration, newAcceleration);

    if (newAcceleration != _vehicleState.acceleration) {
      triggerAccelerationState(newAcceleration, true);
      _lastBrakeUpdate = millis();
      return true;
    }
  }

  return false;
}

bool Motion::detectTurning() {
  TurnState newTurn;
  float angle = getAttitudeFromAxis(_turnAxis);
  float turnDelta = abs(_turnZero - angle);
  float threshold = _turnThreshold;
  
  if (_vehicleState.orientation == FrontSideUp || _vehicleState.orientation == BackSideUp)
    threshold *= 2.0f;

  if (turnDelta > threshold && turnDelta < 180 - threshold)
    newTurn = angle < 0 ? TurnState::Right : TurnState::Left;
  else
    newTurn = TurnState::Center;

#if defined(LOG_MOTION_TURN_DETECTION)
  Log::verbose("turn angle: abs(%F - %F) %F > %F", _turnZero, angle, abs(_turnZero - angle), _turnThreshold);
#endif

  if (newTurn != _vehicleState.turn) {
    triggerTurnState(newTurn, true);
    return true;
  }

  return false;
}

bool Motion::detectOrientation() {
  Orientation newOrientation = Orientation::UnknownSideUp;

  if (absoluteGravity.x > absoluteGravity.y && absoluteGravity.x > absoluteGravity.z)
    newOrientation = gravity.x > 0 ? FrontSideUp : BackSideUp;
  else if (absoluteGravity.y > absoluteGravity.x && absoluteGravity.y > absoluteGravity.z)
    newOrientation = gravity.y > 0 ? RightSideUp : LeftSideUp;
  else if (absoluteGravity.z > absoluteGravity.x && absoluteGravity.z > absoluteGravity.y)
    newOrientation = gravity.z > 0 ? TopSideUp : BottomSideUp;

  if (newOrientation != _vehicleState.orientation) {
    triggerOrientationState(newOrientation, true);
    return true;
  }
  
  return false;
}

float Motion::getAccelerationFromAxis(AccelerationAxis axis) {
  float acceleration = 0.0f;

  switch (axis) {
    case AccelerationAxis::X_Neg:
      acceleration = -linearAcceleration.x;
      break;
    case AccelerationAxis::X_Pos:
      acceleration = linearAcceleration.x;
      break;
    case AccelerationAxis::Y_Neg:
      acceleration = -linearAcceleration.y;
      break;
    case AccelerationAxis::Y_Pos:
      acceleration = linearAcceleration.y;
      break;
    case AccelerationAxis::Z_Neg:
      acceleration = -linearAcceleration.z;
      break;
    case AccelerationAxis::Z_Pos:
      acceleration = linearAcceleration.z;
      break;
  }

  return acceleration;
}

float Motion::getAttitudeFromAxis(AttitudeAxis axis) {
  float angle = 0.0f;

  switch (axis) {
    case AttitudeAxis::Roll:
      angle = attitude.x;
      break;
    case AttitudeAxis::Roll_Invert:
      angle = -attitude.x;
      break;
    case AttitudeAxis::Pitch:
      angle = attitude.y;
      break;
    case AttitudeAxis::Pitch_Invert:
      angle = -attitude.y;
      break;
    case AttitudeAxis::Yaw:
      angle = attitude.z;
      break;
    case AttitudeAxis::Yaw_Invert:
      angle = -attitude.z;
  }

  return angle;
}