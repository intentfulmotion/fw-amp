#pragma once
#include <common.h>
#include <freertos/FreeRTOS.h>
#include "driver/gpio.h"
#include <interfaces/touch-listener.h>

class AmpButton {
  TouchTaskListener *_listener = nullptr;
  long touchEventTimeout = 500;
  long holdDuration = 500;
  long touchStart, touchEnd;
  static bool goingDown;

  public:
    void init(TouchTaskListener *listener);
    void deinit();

    void process();
    static void onButtonInteraction();
    static QueueHandle_t buttonEventQueue;
};