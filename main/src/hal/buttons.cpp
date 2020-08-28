#include <hal/buttons.h>

void Buttons::onPowerUp() {
  ampButtons.init(this);
  xTaskCreatePinnedToCore(inputTask, "buttons", 2048, this, 2, &inputTaskHandle, 1);
}

void Buttons::onPowerDown() {
  vTaskDelete(inputTaskHandle);

  ampButtons.deinit();
}

void Buttons::process() {
  ampButtons.process();

  if (touchEnd > touchStart && millis() - touchEnd > touchEventTimeout && touches.size() > 0) {
    Log::trace("Touch interaction ended. Touches: %d", touches.size());

    for (auto listener : touchListeners)
      if (listener->touchEventQueue != NULL)
        xQueueSendToBack(listener->touchEventQueue, &touches, 0);

    // reset touches
    touches.clear();
  }
}

void Buttons::onTouchDown() {
  touchStart = millis();
  Log::trace("Primary Button: Pressed");

  bool touch = true;
  for (auto listener : touchListeners)
    if (listener->touchQueue != NULL)
      xQueueSendToBack(listener->touchQueue, &touch, 0);
}

void Buttons::onTouchUp() {
  if (touchEnd < touchStart) {
    touchEnd = millis();
    Log::trace("Primary Button: Released");

    bool touch = false;
    for (auto listener : touchListeners)
      if (listener->touchQueue != NULL)
        xQueueSendToBack(listener->touchQueue, &touch, 0);
      // listener->onTouchUp();

    long duration = touchEnd - touchStart;
    TouchType type = duration < holdDuration ? Tap : Hold;
    touches.push_back(type);
  }
}

void Buttons::addTouchListener(TouchListener *listener) {
  touchListeners.push_back(listener);
}

void Buttons::inputTask(void *parameters) {
  Buttons *buttons = (Buttons*)parameters;

  for (;;) {
    Power::powerDown.wait("power");
    buttons->process();
    delay(10);
  }
}