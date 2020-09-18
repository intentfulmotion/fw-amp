#pragma once

// #include <mutex>
// #include <thread>
// #include <chrono>

// #include <driver/spi_master.h>
// #include <driver/gpio.h>
// #include <nvs_flash.h>
// #include <nvs.h>
// #include <esp_log.h>

// #include <freertos/FreeRTOS.h>
// #include <freertos/task.h>

// GPIO Pins
// LED Channels
#define LED_STATUS              GPIO_NUM_13
#define LED_CHANNEL_1_DATA      GPIO_NUM_21
#define LED_CHANNEL_1_CLK       GPIO_NUM_22

#define LED_CHANNEL_2_DATA      GPIO_NUM_33
#define LED_CHANNEL_2_CLK       GPIO_NUM_25

#define LED_CHANNEL_3_DATA      GPIO_NUM_26
#define LED_CHANNEL_3_CLK       GPIO_NUM_27

#define LED_CHANNEL_4_DATA      GPIO_NUM_14
#define LED_CHANNEL_4_CLK       GPIO_NUM_12

// Input
#define AMP_INPUT               GPIO_NUM_36

// Power
#define POWER_HOLD              GPIO_NUM_32
#define VBAT_SENSE              GPIO_NUM_39

#define PARALLEL_LINES          16
#define NVS_STORAGE_NAME        "amp"

#define ESP_INTR_FLAG_DEFAULT   0

// ColorRGBs
// #define AMP_PINK                CRGB(255, 33, 125)
// #define AMP_PURPLE              CRGB(134, 23, 237)
// #define AMP_ORANGE              CRGB(240, 109, 0)

// extern spi_device_handle_t spi;
// extern std::mutex nvs_mutex;
// extern std::chrono::microseconds prefsDelay;

// esp_err_t configure_pins();
// esp_err_t init_motion();
// esp_err_t init_nvs();
// esp_err_t setup_board();

// void power_up();
// void power_down();

// esp_err_t open_prefs(nvs_handle_t *handle);
// void close_prefs(nvs_handle_t handle);

// void ble_host_task(void *param);