#include "NimBLEDevice.h"
#include "amp-main.h"

void appLoop(void *params) {
  amp->init();
  
  for (;;) {
    amp->process();
    delay(50);
  }

  vTaskDelete(NULL);
}

extern "C" void app_main(void) {
  xTaskCreatePinnedToCore(appLoop, "app_main", 4096, NULL, 5, &mainHandle, 1);
}