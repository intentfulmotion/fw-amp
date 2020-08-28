#include <services/battery-service.h>

BatteryService::BatteryService(BLEServer *server) {
  _server = server;
  powerStatusQueue = xQueueCreate(1, sizeof(PowerStatus));

  setupService();
}

void BatteryService::process() {
  if (uxQueueMessagesWaiting(powerStatusQueue)) {
    xQueueReceive(powerStatusQueue, &_powerStatus, 0);
    onPowerStatusChanged(_powerStatus);
  }
}

void BatteryService::setupService() {
  auto service = _server->createService(NimBLEUUID((uint16_t) 0x180F));

  _batteryCharacteristic = service->createCharacteristic(
    // https://www.bluetooth.com/wp-content/uploads/Sitecore-Media-Library/Gatt/Xml/Characteristics/org.bluetooth.characteristic.battery_level_state.xml
    NimBLEUUID((uint16_t) 0x2A1B),
    NIMBLE_PROPERTY::READ |
    NIMBLE_PROPERTY::NOTIFY);

  service->start();
}

void BatteryService::onPowerStatusChanged(PowerStatus status) {
  uint8_t data[2];
  data[0] = status.percentage;

  // present or not
  data[1] = status.batteryPresent ? 0x03 : 0x02;

  // discharging or not
  if (!status.batteryPresent)
    data[1] |= 0x02 << 2;
  else
    data[1] |= status.charging ? 0x02 << 2 : 0x03 << 2;

  // charging or not
  if (!status.batteryPresent)
    data[1] |= 0x02 << 4;
  else
    data[1] |= status.charging ? 0x03 << 4 : 0x02 << 4;

  switch (status.level) {
    case PowerLevel::Unknown:
      data[1] |= 0x00 << 6;
      break;
    case PowerLevel::Normal:
    case PowerLevel::Low:
    case PowerLevel::Charged:
      data[1] |= 0x02 << 6;
      break;
    case PowerLevel::Critical:
      data[1] |= 0x03 << 6;
      break;
  }

  _batteryCharacteristic->setValue(data, sizeof(data));
  _batteryCharacteristic->notify();
}