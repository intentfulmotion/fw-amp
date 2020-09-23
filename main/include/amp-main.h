#pragma once
#include <amp.h>
#include <esp_log.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_task_wdt.h"

Amp *amp = Amp::instance();
TaskHandle_t render;
TaskHandle_t mainHandle;