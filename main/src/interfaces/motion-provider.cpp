#include <interfaces/motion-provider.h>

MotionProvider::MotionProvider()
{
  configUpdatedQueue = xQueueCreate(1, sizeof(bool));
}

void MotionProvider::addMotionListener(MotionListener *listener)
{
  motionListeners.push_back(listener);
}

void MotionProvider::removeMotionListener(MotionListener *listener)
{
  motionListeners.erase(std::remove(motionListeners.begin(), motionListeners.end(), listener));
}

void MotionProvider::notifyMotionListeners()
{
  // notify any listeners of the reset
  for (auto listener : motionListeners)
  {
    // vehicle state changed
    if (listener->vehicleQueue != NULL)
      xQueueSendToBack(listener->vehicleQueue, &_vehicleState, 0);
  }
}

void MotionProvider::resetMotionDetection()
{
  if (Config::ampConfig.motion.autoMotion)
    _vehicleState.acceleration = AccelerationState::Neutral;

  if (Config::ampConfig.motion.autoTurn)
    _vehicleState.turn = TurnState::Center;

  if (Config::ampConfig.motion.autoOrientation)
    _vehicleState.orientation = Orientation::UnknownSideUp;

  if (Config::ampConfig.motion.autoDirection)
    _vehicleState.direction = Direction::Forward;

  notifyMotionListeners();
}

void MotionProvider::setMotionDetection(bool enabled, AccelerationAxis axis, float brakeTreshold, float accelerationTreshold)
{
  _autoMotion = enabled;
  _motionAxis = axis;
  _brakeThreshold = brakeTreshold;
  _accelerationThreshold = accelerationTreshold;

  if (_autoMotion || _autoTurn || _autoOrientation || _autoDirection)
    _enabled = true;
  else if (!_autoMotion && !_autoTurn && !_autoOrientation && !_autoDirection)
    _enabled = false;
}

void MotionProvider::setTurnDetection(bool enabled, bool useRelativeTurnZero, AttitudeAxis axis, float threshold)
{
  _autoTurn = enabled;
  _useRelativeTurnZero = useRelativeTurnZero;
  _turnAxis = axis;
  _turnThreshold = threshold;

  if (_autoMotion || _autoTurn || _autoOrientation || _autoDirection)
    _enabled = true;
  else if (!_autoMotion && !_autoTurn && !_autoOrientation && !_autoDirection)
    _enabled = false;
}

void MotionProvider::setOrientationDetection(bool enabled, Orientation trigger)
{
  _autoOrientation = enabled;
  _orientationTrigger = trigger;

  if (_autoMotion || _autoTurn || _autoOrientation || _autoDirection)
    _enabled = true;
  else if (!_autoMotion && !_autoTurn && !_autoOrientation && !_autoDirection)
    _enabled = false;
}

void MotionProvider::setDirectionDetection(bool enabled, DirectionTrigger directionTrigger, AccelerationAxis accelAxis, AttitudeAxis attitudeAxis, float directionThreshold)
{
  _autoDirection = enabled;
  _directionTrigger = directionTrigger;
  _directionAccelerationAxis = accelAxis;
  _directionAttitudeAxis = attitudeAxis;

  if (_autoMotion || _autoTurn || _autoOrientation || _autoDirection)
    _enabled = true;
  else if (!_autoMotion && !_autoTurn && !_autoOrientation && !_autoDirection)
    _enabled = false;
}

void MotionProvider::triggerVehicleState(VehicleState state, bool autoMotion, bool autoTurn, bool autoOrient, bool autoDirection)
{
  _autoMotion = autoMotion;
  _autoTurn = autoTurn;
  _autoOrientation = autoOrient;
  _autoDirection = autoDirection;

  _vehicleState = state;
  ESP_LOGV(MOTION_TAG, "vehicle state change. accel: %d", state.acceleration);
  notifyMotionListeners();
}

void MotionProvider::triggerAccelerationState(AccelerationState state, bool autoMotion)
{
  _vehicleState.acceleration = state;
  triggerVehicleState(_vehicleState, autoMotion, _autoTurn, _autoOrientation, _autoDirection);
}

void MotionProvider::triggerTurnState(TurnState state, bool autoTurn)
{
  _vehicleState.turn = state;
  triggerVehicleState(_vehicleState, _autoMotion, autoTurn, _autoOrientation, _autoDirection);
}

void MotionProvider::triggerOrientationState(Orientation state, bool autoOrientation)
{
  _vehicleState.orientation = state;
  triggerVehicleState(_vehicleState, _autoMotion, _autoTurn, autoOrientation, _autoDirection);
}

void MotionProvider::triggerDirectionState(Direction state, bool autoDirection)
{
  _vehicleState.direction = state;
  triggerVehicleState(_vehicleState, _autoMotion, _autoTurn, _autoOrientation, autoDirection);
}

void MotionProvider::onConfigUpdated()
{
  ESP_LOGI(MOTION_TAG, "Updating motion for config");
  auto motion = Config::ampConfig.motion;

  // update motion detection
  if (!motion.autoMotion && !motion.autoOrientation && !motion.autoTurn && !motion.autoDirection)
    _enabled = false;
  else
    _enabled = true;

  setMotionDetection(motion.autoMotion, motion.motionAxis, motion.brakeThreshold, motion.accelerationThreshold);
  setTurnDetection(motion.autoTurn, motion.relativeTurnZero, motion.turnAxis, motion.turnThreshold);
  setOrientationDetection(motion.autoOrientation, motion.orientationTrigger);
  setDirectionDetection(motion.autoDirection, motion.directionTriggerType, motion.directionAccelerationAxis, motion.directionAttitudeAxis, motion.directionAttitudeAxis);

  resetMotionDetection();
}