#pragma once
#include <NimBLEService.h>
#include <hal/ble.h>

#include <vector>
#include <string>
#include <constants.h>
#include <hal/config.h>

static const char* CONFIG_SERVICE_TAG = "config-service";

class ConfigService : public NimBLECharacteristicCallbacks {
  Config *_config;
  NimBLEServer *_server;

  std::string rxBuffer;
  uint16_t packetSize;
  
  NimBLECharacteristic *_configRxCharacteristic;
  NimBLECharacteristic *_configTxCharacteristic;

  public:
    ConfigService(Config *config, NimBLEServer *server);

    void setupService();
    void onWrite(NimBLECharacteristic *characteristic);

    void processCommand(std::string data);
    void transmit(std::string data);
    void notify(uint16_t conn_id, std::string data, bool notify);
    std::vector<std::string> buildPackets(std::string data, size_t packetSize);
    
    FreeRTOS::Semaphore eventSemaphore = FreeRTOS::Semaphore("configEvents");
};