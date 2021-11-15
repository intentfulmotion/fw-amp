#pragma once
#include <common.h>
#ifdef HAS_VESC_CAN

#include <queue>
#include <FreeRTOS.h>
#include <models/vesc-status.h>
#include <interfaces/lifecycle.h>
#include <interfaces/motion-provider.h>
#include <interfaces/power-provider.h>

static const char* CAN_TAG = "can";
#define BUFFER_SIZE 65535

#define RECV_STATUS_1 (uint32_t(0x0000) << 16) + (uint16_t(CAN_PACKET_STATUS) << 8) + VESC_ID
#define RECV_STATUS_2 (uint32_t(0x0000) << 16) + (uint16_t(CAN_PACKET_STATUS_2) << 8) + VESC_ID
#define RECV_STATUS_3 (uint32_t(0x0000) << 16) + (uint16_t(CAN_PACKET_STATUS_3) << 8) + VESC_ID
#define RECV_STATUS_4 (uint32_t(0x0000) << 16) + (uint16_t(CAN_PACKET_STATUS_4) << 8) + VESC_ID
#define RECV_STATUS_5 (uint32_t(0x0000) << 16) + (uint16_t(CAN_PACKET_STATUS_5) << 8) + VESC_ID

#define RECV_FILL_RX_BUFFER (uint32_t(0x0000) << 16) + (uint16_t(CAN_PACKET_FILL_RX_BUFFER) << 8) + AMP_ID
#define RECV_PROCESS_RX_BUFFER (uint32_t(0x0000) << 16) + (uint16_t(CAN_PACKET_PROCESS_RX_BUFFER) << 8) + AMP_ID

#define RECV_PROCESS_SHORT_BUFFER_PROXY (uint32_t(0x0000) << 16) + (uint16_t(CAN_PACKET_PROCESS_SHORT_BUFFER) << 8) + BLE_PROXY_ID
#define RECV_FILL_RX_BUFFER_PROXY (uint32_t(0x0000) << 16) + (uint16_t(CAN_PACKET_FILL_RX_BUFFER) << 8) + BLE_PROXY_ID
#define RECV_FILL_RX_BUFFER_LONG_PROXY (uint32_t(0x0000) << 16) + (uint16_t(CAN_PACKET_FILL_RX_BUFFER_LONG) << 8) + BLE_PROXY_ID
#define RECV_PROCESS_RX_BUFFER_PROXY (uint32_t(0x0000) << 16) + (uint16_t(CAN_PACKET_PROCESS_RX_BUFFER) << 8) + BLE_PROXY_ID

class VescCan : public MotionProvider, public PowerProvider {
  TaskHandle_t canHandle;
  bool initialized;
  VescStatus status;

  std::vector<uint8_t> buffer = {};
  std::vector<uint8_t> proxybuffer = {};
  int interval = 200;
  SemaphoreHandle_t mutex_v = xSemaphoreCreateMutex();
  int initRetryCounter = 5;
  int frameCount = 0;
  int lastRetry = 0;
  int lastPrint = 0;
  uint16_t length = 0;
  uint8_t command = 0;
  bool longPacket = false;
  std::string longPackBuffer;

  void requestFirmwareVersion();
  void requestRealtimeData();
  void requestBalanceData();
  void ping();
  int16_t readInt16(can_message_t message, int start);
  int32_t readInt32(can_message_t message, int start);
  int8_t readInt8FromBuffer(int start, bool isProxyRequest);
  int16_t readInt16FromBuffer(int start, bool isProxyRequest);
  int32_t readInt32FromBuffer(int start, bool isProxyRequest);
  std::string readStringFromBuffer(int start, int length, bool isProxyRequest);

  bool _autoMotion, _autoTurn, _autoOrientation;

  public:
    static VescCan* instance() { static VescCan can; return &can; }
    VescCan();

    static void canTask(void* params);
    void processFrame(can_message_t message);
    void processBufferedData(uint8_t command, bool isProxyRequest);
    void proxyIn(std::string in);
    void proxyOut(uint8_t *data, int size, uint8_t crc1, uint8_t crc2);
    void sendFrame(can_message_t *message);

    void parseFirmwareData(bool isProxyRequest);
    void parseBalanceData(bool isProxyRequest);
    void parseRealtimeData(bool isProxyRequest);

    float getDirectionFromAxis();

    QueueHandle_t transmitQueue = NULL;
    QueueHandle_t receiveQueue = NULL;

    static FreeRTOS::Semaphore canReceive;
    static FreeRTOS::Semaphore canTransmit;

    // motion provider
    void addMotionListener(MotionListener listener);
    void resetMotionDetection();

    void setMotionDetection(bool enabled, AccelerationAxis axis, float brakeThreshold, float acclerationThreshold);
    void setTurnDetection(bool enabled, bool useRelativeTurnZero, AttitudeAxis axis, float threshold);
    void setOrientationDetection(bool enabled, Orientation orientationTrigger);

    bool detectMotion();
    bool detectTurning() { return false; }
    bool detectOrientation() { return false; }
    bool detectDirection();

    // motion triggers
    void triggerVehicleState(VehicleState state, bool autoMotion = false, bool autoTurn = false, bool autoOrient = false);
    void triggerAccelerationState(AccelerationState state, bool autoMotion = false);
    void triggerTurnState(TurnState state, bool autoTurn = false);
    void triggerOrientationState(Orientation state, bool autoOrient = false);

    bool isMotionDetectionEnabled();
    bool isTurnDetectionEnabled();
    bool isOrientationDetectionEnabled();

    // power status listener
    void onPowerStatusChanged(PowerStatus status) { }

    void startup();
    void process();
    void shutdown(bool restart = false) {}   

    // motionprovider lifecycle base
    void onPowerUp() {}
    void onPowerDown() {}
};

#endif