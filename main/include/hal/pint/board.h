#pragma once

#include "driver/gpio.h"
#include "driver/can.h"
#include "models/vesc-status.h"

// Status LED
#define STATUS_LED 13
#define STATUS_ENABLED false

// I/O Channels
#define STRIP_ONE_DATA 32
#define STRIP_ONE_CLK 21
#define STRIP_TWO_DATA 33
#define STRIP_TWO_CLK 25
#define STRIP_THREE_DATA 27
#define STRIP_THREE_CLK 26
#define STRIP_FOUR_DATA 14
#define STRIP_FOUR_CLK 12

// Input
#define BUTTON_INPUT GPIO_NUM_36

// Power Management
#define POWER_HOLD GPIO_NUM_32
#define VBAT_SENSE GPIO_NUM_39
#define BAT_CHRG GPIO_NUM_34
#define BAT_DONE GPIO_NUM_35

// CAN BUS
#define CAN_TX GPIO_NUM_23
#define CAN_RX GPIO_NUM_22
#define VESC_ID 1
#define AMP_ID 2
#define BLE_PROXY_ID 3

#define BLE_ENABLED
#define HAS_VESC_CAN

class Board
{
public:
  void setupBoard();
};