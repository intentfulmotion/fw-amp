#pragma once
#include <NimBLEService.h>
#include <hal/ble.h>
#include <hal/vesc-can.h>
#include <constants.h>

class VescService : public NimBLECharacteristicCallbacks {
  VescCan *_can;
  NimBLEServer *_server;
  NimBLECharacteristic *_txCharacteristic;
  NimBLECharacteristic *_rxCharacteristic;
  
  public:
    VescService(VescCan *can, NimBLEServer *server);

    void setupService();
    void onWrite(NimBLECharacteristic *characteristic);
    void process();
    void transmit(std::string data);
    std::vector<std::string> buildPackets(std::string data, size_t packetSize);
    void notify(uint16_t conn_id, std::string value, bool is_notification);
};