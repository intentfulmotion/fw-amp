#pragma once
#include <common.h>
#include "esp_ota_ops.h"
#include "esp_system.h"
#include <vector>
#include "interfaces/update-listener.h"

static const char* UPDATER_TAG = "ota";

class Updater {
  std::vector<UpdateListener*> updateListeners;
  UpdateStatus status;

  const esp_partition_t *updatePartition;
  esp_ota_handle_t updateHandle;
  uint16_t updatePacketsCount;
  size_t dataLength;

  void notifyUpdateListeners();

  public:
    static Updater* instance() { static Updater updater; return &updater; }

    void startUpdate();
    void endUpdate();
    void writeUpdate(std::string data);

    void addUpdateListener(UpdateListener *listener);
};