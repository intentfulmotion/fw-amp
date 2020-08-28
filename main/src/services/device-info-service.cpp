#include <services/device-info-service.h>

DeviceInfoService::DeviceInfoService(NimBLEServer *server) {
  _server = server;

  setupService();
}

void DeviceInfoService::setupService() {
  auto service = _server->createService(NimBLEUUID((uint16_t) 0x180A));

  _manufacturerCharacteristic = service->createCharacteristic(
    NimBLEUUID((uint16_t) 0x2A29),
    NIMBLE_PROPERTY::READ);

  _firmwareVersionCharacteristic = service->createCharacteristic(
    NimBLEUUID((uint16_t) 0x2A26),
    NIMBLE_PROPERTY::READ);

  _hardwareVersionCharacteristic = service->createCharacteristic(
    NimBLEUUID((uint16_t) 0x2A27),
    NIMBLE_PROPERTY::READ);
  
  _serialNumberCharacteristic = service->createCharacteristic(
    NimBLEUUID((uint16_t) 0x2A25),
    NIMBLE_PROPERTY::READ);

  // set device info
  auto info = Config::getDeviceInfo();
  _manufacturerCharacteristic->setValue(info.manufacturer);
  _firmwareVersionCharacteristic->setValue(info.firmwareVersion);
  _hardwareVersionCharacteristic->setValue(info.hardwareVersion);
  _serialNumberCharacteristic->setValue(info.serialNumber);

  service->start();
}