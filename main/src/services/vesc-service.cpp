#include <services/vesc-service.h>

VescService::VescService(VescCan *can, NimBLEServer *server)
{
  _can = can;
  _server = server;

  setupService();
}

void VescService::setupService()
{
  auto service = _server->createService(vescServiceUUID);

  _rxCharacteristic = service->createCharacteristic(
      NimBLEUUID::fromString(vescRxCharacteristicUUID),
      NIMBLE_PROPERTY::READ |
          NIMBLE_PROPERTY::READ_ENC |
          NIMBLE_PROPERTY::WRITE |
          NIMBLE_PROPERTY::WRITE_NR |
          NIMBLE_PROPERTY::WRITE_ENC);

  _rxCharacteristic->setCallbacks(this);

  _txCharacteristic = service->createCharacteristic(
      NimBLEUUID::fromString(vescTxCharacteristicUUID),
      NIMBLE_PROPERTY::READ |
          NIMBLE_PROPERTY::READ_ENC |
          NIMBLE_PROPERTY::NOTIFY);

  service->start();
}

void VescService::onWrite(NimBLECharacteristic *characteristic)
{
  auto uuid = characteristic->getUUID();
  std::string dataStr = characteristic->getValue();

  if (uuid.equals(_rxCharacteristic->getUUID()))
  {
    std::string *buffer = new std::string(dataStr);
    VescCan::canReceive.wait();
    VescCan::canReceive.take();
    xQueueSendToBack(_can->receiveQueue, &buffer, 0);
  }
}

void VescService::process()
{
  if (uxQueueMessagesWaiting(_can->transmitQueue))
  {
    std::string *buffer = NULL;
    xQueueReceive(_can->transmitQueue, &buffer, 0);
    transmit(*buffer);
    delete buffer;
    VescCan::canTransmit.give();
  }
}

void VescService::transmit(std::string data)
{
  auto m_properties = _txCharacteristic->m_properties;
  auto m_subscribedVec = _txCharacteristic->m_subscribedVec;
  bool is_notification = true;

  if (m_subscribedVec.size() == 0)
  {
    return;
  }

  _txCharacteristic->m_pCallbacks->onNotify(_txCharacteristic);

  bool reqSec = (m_properties & BLE_GATT_CHR_F_READ_AUTHEN) ||
                (m_properties & BLE_GATT_CHR_F_READ_AUTHOR) ||
                (m_properties & BLE_GATT_CHR_F_READ_ENC);
  int rc = 0;

  for (auto &it : m_subscribedVec)
  {
    uint16_t _mtu = _txCharacteristic->getService()->getServer()->getPeerMTU(it.first);
    uint16_t packetSize = _mtu - 3;

    // check if connected and subscribed
    if (_mtu == 0 || it.second == 0)
      continue;

    // check if security requirements are satisfied
    if (reqSec)
    {
      struct ble_gap_conn_desc desc;
      rc = ble_gap_conn_find(it.first, &desc);
      if (rc != 0 || !desc.sec_state.encrypted)
        continue;
    }

    if (!(it.second & 0x0001))
      is_notification = false;

    if (!(it.second & 0x0002))
      is_notification = true;

    // send packets
    auto packets = buildPackets(data, packetSize);
    for (auto packet : packets)
    {
      notify(it.first, packet, is_notification);
      delay(10);
    }
  }
}

void VescService::notify(uint16_t conn_id, std::string value, bool is_notification)
{
  int rc = 0;
  auto m_properties = _txCharacteristic->m_properties;
  auto m_handle = _txCharacteristic->m_handle;

  // don't create the m_buf until we are sure to send the data or else
  // we could be allocating a buffer that doesn't get released.
  // We also must create it in each loop iteration because it is consumed with each host call.
  os_mbuf *om = ble_hs_mbuf_from_flat((uint8_t *)value.data(), value.length());

  NimBLECharacteristicCallbacks::Status statusRC;

  if (!is_notification && (m_properties & NIMBLE_PROPERTY::INDICATE))
  {
    ble_task_data_t taskData = {nullptr, xTaskGetCurrentTaskHandle(), 0, nullptr};
    _txCharacteristic->m_pTaskData = &taskData;

    rc = ble_gattc_indicate_custom(conn_id, m_handle, om);
    if (rc != 0)
      statusRC = NimBLECharacteristicCallbacks::Status::ERROR_GATT;
    else
    {
      ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
      rc = _txCharacteristic->m_pTaskData->rc;
    }

    _txCharacteristic->m_pTaskData = nullptr;

    if (rc == BLE_HS_EDONE)
    {
      rc = 0;
      statusRC = NimBLECharacteristicCallbacks::Status::SUCCESS_INDICATE;
    }
    else if (rc == BLE_HS_ETIMEOUT)
      statusRC = NimBLECharacteristicCallbacks::Status::ERROR_INDICATE_TIMEOUT;
    else
      statusRC = NimBLECharacteristicCallbacks::Status::ERROR_INDICATE_FAILURE;
  }
  else
  {
    rc = ble_gattc_notify_custom(conn_id, m_handle, om);
    if (rc == 0)
      statusRC = NimBLECharacteristicCallbacks::Status::SUCCESS_NOTIFY;
    else
      statusRC = NimBLECharacteristicCallbacks::Status::ERROR_GATT;
  }

  _txCharacteristic->m_pCallbacks->onStatus(_txCharacteristic, statusRC, rc);
}

std::vector<std::string> VescService::buildPackets(std::string data, size_t packetSize)
{
  // calculate how many packets
  size_t packetCount = data.length() / packetSize;
  packetCount = data.length() % packetSize == 0 ? packetCount : packetCount + 1;

  std::vector<std::string> packets;
  // construct each packet
  for (int i = 0; i < packetCount; i++)
  {
    // start of packet
    std::string packet = i == packetCount - 1 ? data.substr(i * packetSize) : data.substr(i * packetSize, packetSize);
    // add to vector of packets
    packets.push_back(packet);
  }

  return packets;
}