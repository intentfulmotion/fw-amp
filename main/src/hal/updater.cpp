#include <updater.h>

void Updater::startUpdate() {
  status = UpdateStatus::Start;
  updatePacketsCount = 0;
  
  // get next ota partition
  updatePartition = esp_ota_get_next_update_partition(NULL);

  // start update + set update handle
  auto error = esp_ota_begin(updatePartition, OTA_SIZE_UNKNOWN, &updateHandle);
  if (error != ESP_OK)
    status = UpdateStatus::ErrorStart;

  notifyUpdateListeners();
}

void Updater::endUpdate() {
  status = UpdateStatus::End;
  ESP_LOGV(UPDATER_TAG,"Ending update. Total written: %d bytes", dataLength);

  // end the ota update and verify it's good
  auto error = esp_ota_end(updateHandle);
  if (error != ESP_OK) {
    status = UpdateStatus::ErrorEnd;
    ESP_LOGV(UPDATER_TAG,"Error ending update: %d", status);
  }
  else {
    // set next boot partition
    error = esp_ota_set_boot_partition(updatePartition);
    if (error != ESP_OK)
      status = UpdateStatus::ErrorEnd;
  }

  notifyUpdateListeners();
}

void Updater::writeUpdate(std::string data) {
  updatePacketsCount++;
  ESP_LOGV(UPDATER_TAG,"Update packet %d", updatePacketsCount);

  const char* dataC = data.data();
  auto size = data.length();
  dataLength += size;

  printf("Data: ");
  for (int i = 0; i < size; i++)
    printf("%d ", dataC[i]);
  printf("\n");
  
  auto error = esp_ota_write(updateHandle, (const void*) dataC, size);
  if (error != ESP_OK) {
    status = UpdateStatus::ErrorWrite;
    ESP_LOGV(UPDATER_TAG,"Write error: %d", status);
  }
  
  notifyUpdateListeners();
}

void Updater::addUpdateListener(UpdateListener *listener) {
  updateListeners.push_back(listener);
}

void Updater::notifyUpdateListeners() {
  for(auto listener : updateListeners)
    if (listener->updateStatusQueue != NULL)
      xQueueSend(listener->updateStatusQueue, &status, 0);
}