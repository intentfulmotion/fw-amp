#pragma once
#include <stdint.h>
#include <assert.h>
#include <map>
#include <string>
#include <cstring>

enum IMUState : uint8_t {
  IMU_Error = 0,
  IMU_Disabled,
  IMU_LowPower,
  IMU_Normal,
};

enum AccelerationState : uint8_t {
  Neutral = 0,
  Braking,
  Accelerating
};

enum TurnState : uint8_t {
  Center = 0,
  Left,
  Right,
  Hazard
};

enum Orientation : uint8_t {
  UnknownSideUp,
  TopSideUp,
  BottomSideUp,
  LeftSideUp,
  RightSideUp,
  FrontSideUp,
  BackSideUp
};

struct VehicleState {
  AccelerationState acceleration;
  TurnState turn;
  Orientation orientation;

  VehicleState& operator=(VehicleState const &other) {
    std::memcpy(&acceleration, &other.acceleration, sizeof(AccelerationState));
    std::memcpy(&turn, &other.turn, sizeof(TurnState));
    std::memcpy(&orientation, &other.orientation, sizeof(Orientation));
    return *this;
  }
};

enum CalibrationState : uint8_t {
  Started = 0,
  Ended = 1
};

inline bool operator==(const VehicleState& lhs, const VehicleState& rhs) {
  return lhs.acceleration == rhs.acceleration && lhs.turn == rhs.turn && lhs.orientation == rhs.orientation;
}

inline bool operator!=(const VehicleState& lhs, const VehicleState& rhs) {
  return lhs.acceleration != rhs.acceleration || lhs.turn != rhs.turn || lhs.orientation != rhs.orientation;
}

static std::map<AccelerationState, std::string> AccelerationStateMap = {
  { Braking, "Braking" },
  { Accelerating, "Accelerating" },
  { Neutral, "Neutral" }
};

static std::map<TurnState, std::string> TurnStateMap = {
  { Left, "Left" },
  { Right, "Right" },
  { Center, "Center" },
  { Hazard, "Hazard" }
};

static std::map<Orientation, std::string> OrientationMap = {
  { UnknownSideUp, "Unknown Orientation" },
  { TopSideUp, "Top Side Up" },
  { BottomSideUp, "Bottom Side Up" },
  { LeftSideUp, "Left Side Up" },
  { RightSideUp, "Right Side Up" },
  { FrontSideUp, "Front Side Up" },
  { BackSideUp, "Back Side Up" },
};

struct Vector3D {
  float x = 0.0f;
  float y = 0.0f;
  float z = 0.0f;

  float& operator[](int index) {
    assert(index >=0 && index < 3);
    if (index == 0) return x;
    else if (index == 1) return y;
    else return z;
  }

  Vector3D& operator-(Vector3D const &other) {
    this->x = this->x - other.x;
    this->y = this->y - other.y;
    this->z = this->z - other.z;
    return *this;
  }

  Vector3D& operator=(Vector3D const &other) {
    std::memcpy(&x, &other.x, sizeof(float));
    std::memcpy(&y, &other.y, sizeof(float));
    std::memcpy(&z, &other.z, sizeof(float));
    return *this;
  }
};

enum AccelerationAxis : uint8_t {
  X_Pos = 0,
  X_Neg,
  Y_Pos,
  Y_Neg,
  Z_Pos,
  Z_Neg
};

enum AttitudeAxis : uint8_t {
  Roll = 0,
  Roll_Invert,
  Pitch,
  Pitch_Invert,
  Yaw,
  Yaw_Invert
};

struct MotionConfig {
  bool autoOrientation;
  bool autoMotion;
  bool autoTurn;
  bool relativeTurnZero;
  float turnZero;
  float turnThreshold;
  float brakeThreshold;
  AccelerationAxis brakeAxis;
  AttitudeAxis turnAxis;
  AttitudeAxis orientationAxis;
  uint16_t orientationUpMin;
  uint16_t orientationUpMax;
};