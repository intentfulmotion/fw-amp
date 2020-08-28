#pragma once

#include "lis3dh.h"
#include <common.h>
#include <models/motion.h>

class AmpIMU {
  static lis3dh_sensor_t* sensor;
  lis3dh_float_data_t data;
  IMUState imuStatus;

  Vector3D accel, gyro, mag;

  public:
    AmpIMU();

    uint8_t init();
    void deinit();
    void process();

    Vector3D getAccelData();
    Vector3D getGyroData();
    Vector3D getMagData();

    bool accelAvailable();
    bool gyroAvailable() { return false; }
    bool magAvailable() { return false; }

    void setPowerMode(IMUState state);

    void calibrateMag(Vector3D *outOffsets);
    void calibrateXG(Vector3D *outOffsets);

    IMUState getIMUState() { return imuStatus; }
};