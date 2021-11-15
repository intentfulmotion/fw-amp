#pragma once

#ifdef HAS_INTERNAL_IMU

// #define LOG_MOTION_RAW_ACCELERATION
// #define LOG_MOTION_RAW_GYRO
// #define LOG_MOTION_GRAVITY
// #define LOG_MOTION_LINEAR_ACCELERATION
// #define LOG_MOTION_AHRS
// #define LOG_MOTION_AHRS_COMPENSATED
// #define LOG_SAMPLE_RATE

// #define USE_MADGWICK_FILTER

#include <common.h>
#include <interfaces/lifecycle.h>
#include <interfaces/power-listener.h>
#include <interfaces/config-listener.h>
#include <interfaces/motion-listener.h>
#include <interfaces/motion-provider.h>
#include <interfaces/calibration-listener.h>
#include <models/motion.h>
#include <models/control.h>
#include "FreeRTOS.h"
#include <hal/common/amp-storage.h>

#if defined(AMP_1_0_x)
  #include <hal/amp-1.0.0/amp-imu.h>
#endif

#include <hal/power.h>
#include <hal/config.h>

#if defined(USE_MADGWICK_FILTER)
  #include <filters/madgwick.h>
#elif defined(USE_SIMPLE_AHRS_FILTER)
  #include <filters/simple-ahrs.h>
#endif

class Motion : public LifecycleBase, public PowerListener, public ConfigListener, public MotionProvider {
  std::vector<CalibrationListener*> calibrationListeners;

  Vector3D rawAccel, rawGyro, rawMag;
  Vector3D linearAcceleration, gravity, absoluteGravity, attitude;
  Vector3D accelBias, gyroBias, magBias;
  IMUState imuState = IMUState::IMU_Disabled;
  static AmpIMU ampIMU;
#if defined(USE_MADGWICK_FILTER)
  Madgwick *filter = nullptr;
#elif defined(USE_SIMPLE_AHRS_FILTER)
  SimpleAHRS *filter = nullptr;
#endif

  float _sampleRate = 0;

  // alpha for high pass filter for linear acceleration / gravity calc
  float _alpha = 0.5f;
  float expFilterWeight = 0.2f;

  // calibrations
  bool _calibrating = false;

  unsigned long _lastUpdate = micros();
  unsigned long _lastSample = micros();

  unsigned long _lastMotionUpdate = millis();
  const unsigned long MOTION_DEBOUNCE = 100;
  const unsigned long MOTION_ACTIVE_DEBOUNCE = 1000;

  // update config
  void updateGravityFilter(float alpha);
  void updateTurnCenter(float turnCenter);

  void sampleSensorOffsets();
  void setSensorOffsets();

  float getAccelerationFromAxis(AccelerationAxis axis);
  float getAttitudeFromAxis(AttitudeAxis axis);
  TaskHandle_t samplerHandle = NULL;

  void calibrateXG();
  void calibrateMag();

  public:
    static Motion* instance() { static Motion motion; return &motion; }
    static QueueHandle_t calibrationRequestQueue;
    static FreeRTOS::Semaphore holdInterface;
    Motion();

    // lifecycle listener
    void onPowerUp();
    void onPowerDown();
    
    // power status listener
    void onPowerStatusChanged(PowerStatus status);
    void updateMotionForPowerStatus(PowerStatus status);

    // config listener
    void onConfigUpdated();

    // register listeners
    void addMotionListener(MotionListener *listener);
    void removeMotionListener(MotionListener *listener);
    void notifyMotionListeners();

    void addCalibrationListener(CalibrationListener *listener);
    void process();
    void sample();

    // attitude calculations
    void calculateAccelerations(Vector3D accel);

    // motion detection
    void resetMotionDetection();

    bool detectMotion();
    bool detectTurning();
    bool detectOrientation();

    void setMotionDetection(bool enabled) { setMotionDetection(enabled, _motionAxis, _brakeThreshold, _accelerationThreshold); }
    void setMotionDetection(bool enabled, AccelerationAxis axis, float brakeThreshold, float acclerationThreshold);

    void setTurnDetection(bool enabled) { setTurnDetection(enabled, _useRelativeTurnZero, _turnAxis, _turnThreshold); }
    void setTurnDetection(bool enabled, bool useRelativeTurnZero, AttitudeAxis axis, float threshold);

    void setOrientationDetection(bool enabled) { setOrientationDetection(enabled, _orientationTrigger); }
    void setOrientationDetection(bool enabled, Orientation orientationTrigger);

    // motion triggers
    void triggerVehicleState(VehicleState state, bool autoMotion = false, bool autoTurn = false, bool autoOrient = false);
    void triggerAccelerationState(AccelerationState state, bool autoMotion = false);
    void triggerTurnState(TurnState state, bool autoTurn = false);
    void triggerOrientationState(Orientation state, bool autoOrient = false);

    bool isMotionDetectionEnabled() { return _autoMotion; }
    bool isTurnDetectionEnabled() { return _autoTurn; }
    bool isOrientationDetectionEnabled() { return _autoOrientation; }

    static void sampleTask(void *parameters);
};

#endif