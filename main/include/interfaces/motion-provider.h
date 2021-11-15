#pragma once
#include "common.h"
#include <hal/config.h>
#include <models/motion.h>
#include "FreeRTOS.h"
#include "lifecycle.h"
#include "motion-listener.h"
#include "power-listener.h"
#include "config-listener.h"

static const char* MOTION_TAG = "motion";

class MotionProvider : public LifecycleBase, public PowerListener, public ConfigListener {
  protected:
    std::vector<MotionListener*> motionListeners;

    VehicleState _vehicleState;
    AccelerationAxis _motionAxis;
    AttitudeAxis _turnAxis;
    AccelerationAxis _directionAccelerationAxis;
    AttitudeAxis _directionAttitudeAxis;
    Orientation _orientationTrigger;
    DirectionTrigger _directionTrigger;
    float _turnZero = 0;
    bool _autoMotion, _autoTurn, _autoOrientation, _autoDirection, _useRelativeTurnZero;
    float _brakeThreshold, _accelerationThreshold, _turnThreshold, _turnCenter, _directionThreshold;
    bool _enabled = false;
    
  public:
    MotionProvider();
    void addMotionListener(MotionListener *listener);
    void removeMotionListener(MotionListener *listener);
    void notifyMotionListeners();
    void resetMotionDetection();

    void setMotionDetection(bool enabled) { setMotionDetection(enabled, _motionAxis, _brakeThreshold, _accelerationThreshold); }
    void setMotionDetection(bool enabled, AccelerationAxis axis, float brakeThreshold, float acclerationThreshold);

    void setTurnDetection(bool enabled) { setTurnDetection(enabled, _useRelativeTurnZero, _turnAxis, _turnThreshold); }
    void setTurnDetection(bool enabled, bool useRelativeTurnZero, AttitudeAxis axis, float threshold);

    void setOrientationDetection(bool enabled) { setOrientationDetection(enabled, _orientationTrigger); }
    void setOrientationDetection(bool enabled, Orientation orientationTrigger);

    void setDirectionDetection(bool enabled) { setDirectionDetection(enabled, _directionTrigger, _directionAccelerationAxis, _directionAttitudeAxis, _directionThreshold); }
    void setDirectionDetection(bool enabled, DirectionTrigger directionTrigger, AccelerationAxis accelAxis, AttitudeAxis attitudeAxis, float directionThreshold);

    void triggerVehicleState(VehicleState state, bool autoMotion = false, bool autoTurn = false, bool autoOrient = false, bool autoDirectio = false);
    void triggerAccelerationState(AccelerationState state, bool autoMotion = false);
    void triggerTurnState(TurnState state, bool autoTurn = false);
    void triggerOrientationState(Orientation state, bool autoOrient = false);
    void triggerDirectionState(Direction state, bool autoDirection = false);

    bool isMotionDetectionEnabled() { return _autoMotion; }
    bool isTurnDetectionEnabled() { return _autoTurn; }
    bool isOrientationDetectionEnabled() { return _autoOrientation; }
    bool isDirectionDetectionEnabled() { return _autoDirection; }

    void onConfigUpdated();

    virtual bool detectMotion() = 0;
    virtual bool detectTurning() = 0;
    virtual bool detectOrientation() = 0;
    virtual bool detectDirection() = 0;
};