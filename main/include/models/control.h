#pragma once

enum AccelControlState : uint8_t {
  AccelAuto = 0x01,
  AccelNormal,
  AccelAccelerating,
  AccelBraking
};

enum TurnControlState : uint8_t {
  TurnAuto = 0x01,
  TurnCenter,
  TurnLeft,
  TurnRight,
  TurnHazard
};

enum OrientationControlState : uint8_t {
  OrientationAuto = 0x01,
  OrientationStatic
};