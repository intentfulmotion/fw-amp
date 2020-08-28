#include "update-service.h"

UpdateService::UpdateService(Updater *updater, NimBLEServer *server) {
  _updater = updater;
  _server = server;  

  // listen to ota status updates
  updateStatusQueue = xQueueCreate(5, sizeof(UpdateStatus));
  updater->addUpdateListener(this);

  setupService();
}

void UpdateService::setupService() {
  auto service = _server->createService(NimBLEUUID::fromString(updateServiceUUID));

  _updateControlCharacteristic = service->createCharacteristic(
    NimBLEUUID::fromString(updateControlCharacteristicUUID),
    NIMBLE_PROPERTY::WRITE | 
    NIMBLE_PROPERTY::WRITE_NR
  );

  _updateControlCharacteristic->setCallbacks(this);

  _updateRxCharacteristic = service->createCharacteristic(
    NimBLEUUID::fromString(updateRxCharacteristicUUID),
    NIMBLE_PROPERTY::WRITE | 
    NIMBLE_PROPERTY::WRITE_NR
  );

  _updateRxCharacteristic->setCallbacks(this);

  _updateStatusCharacteristic = service->createCharacteristic(
    NimBLEUUID::fromString(updateStatusCharacteristicUUID),
    NIMBLE_PROPERTY::READ | 
    NIMBLE_PROPERTY::NOTIFY
  );

  service->start();
}

void UpdateService::process() {
  if (uxQueueMessagesWaiting(updateStatusQueue)) {
    UpdateStatus status;
    xQueueReceive(updateStatusQueue, &status, 0);
    onUpdateStatusChanged(status);
  }
}

void UpdateService::onWrite(NimBLECharacteristic* characteristic) {
  auto uuid = characteristic->getUUID();
  std::string dataStr = characteristic->getValue();

  if (uuid.equals(_updateControlCharacteristic->getUUID())) {
    Log::trace("update control");
    const char* data = dataStr.data();
    size_t len = sizeof(data);

    if (len >= 1) {
      switch (data[0]) {
        case UpdateStatus::Start:
          Log::trace("Start update");
          _updater->startUpdate();
          break;
        case UpdateStatus::End:
          Log::trace("End update");
          _updater->endUpdate();
          break;
        default: break;
      }
    }
  }
  if (uuid.equals(_updateRxCharacteristic->getUUID())) {
    Log::trace("update data - length: %d", dataStr.length());
    _updater->writeUpdate(dataStr);
  }
}

void UpdateService::onUpdateStatusChanged(UpdateStatus status) {
  if (_updateStatus != status) {
    _updateStatusCharacteristic->setValue<UpdateStatus>(status);
    _updateStatusCharacteristic->notify();

    _updateStatus = status;
  }
}