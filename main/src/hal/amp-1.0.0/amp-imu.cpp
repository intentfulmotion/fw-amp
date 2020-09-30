#include <hal/amp-1.0.0/amp-imu.h>

lis3dh_sensor_t* AmpIMU::sensor = NULL;

AmpIMU::AmpIMU() {
  spi_bus_init(VSPI_HOST, IMU_CLK, IMU_MISO, IMU_MOSI);
  // vspi.begin(IMU_MOSI, IMU_MISO, IMU_CLK, 0);
}

uint8_t AmpIMU::init() {
  sensor = lis3dh_init_sensor(VSPI_HOST, 0, IMU_CS);
  return sensor != NULL;
}

void AmpIMU::deinit() {
  // nothing needed
}

void AmpIMU::process() {
  bool newAccel = lis3dh_new_data(sensor);
  if (newAccel && lis3dh_get_float_data(sensor, &data)) {
    accel.x = data.ax;
    accel.y = data.ay;
    accel.z = data.az;
  }
}

bool AmpIMU::accelAvailable() {
  if (sensor != NULL)
    return false;
  
  return lis3dh_new_data(sensor);
}

Vector3D AmpIMU::getAccelData() {
  return accel;
}

Vector3D AmpIMU::getMagData() {
  return mag;
}

Vector3D AmpIMU::getGyroData() {
  return gyro;
}

void AmpIMU::calibrateMag(Vector3D *offsets) {
  ESP_LOGD(IMU_TAG,"Calibrate mag not implemented");
  // imu.calibrateMag(true);

  // offsets->x = imu.mBias[0];
  // offsets->y = imu.mBias[1];
  // offsets->z = imu.mBias[2];
}

void AmpIMU::calibrateXG(Vector3D *outOffsets) {
  ESP_LOGD(IMU_TAG,"Calibrate XG not implemented");
  // imu.calibrate(false);
  
  // outOffsets[0].x = imu.aBias[0];
  // outOffsets[0].y = imu.aBias[1];
  // outOffsets[0].z = imu.aBias[2];

  // outOffsets[1].x = imu.gBias[0];
  // outOffsets[1].y = imu.gBias[1];
  // outOffsets[1].z = imu.gBias[2];
}

void AmpIMU::setPowerMode(IMUState state) {
  imuStatus = state;
  if (sensor != NULL) {
    switch (state) {
      case IMUState::IMU_Disabled:
        lis3dh_set_mode(sensor, lis3dh_power_down, lis3dh_low_power, false, false, false);
        break;
      case IMUState::IMU_LowPower:
        lis3dh_set_mode(sensor, lis3dh_odr_50, lis3dh_low_power, true, true, true);
        break;
      case IMUState::IMU_Normal:
        lis3dh_set_mode(sensor, lis3dh_odr_50, lis3dh_normal, true, true, true);
        break;
      case IMUState::IMU_Error:
      default:
        break;
    }
  }
}