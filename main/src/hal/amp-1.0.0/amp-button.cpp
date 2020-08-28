#include <hal/amp-1.0.0/amp-button.h>

bool AmpButton::goingDown = false;
QueueHandle_t AmpButton::buttonEventQueue = NULL;

static void IRAM_ATTR button_isr_handler(void* args) {
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  uint32_t io = (uint32_t) args;
  xQueueSendFromISR(AmpButton::buttonEventQueue, &io, &xHigherPriorityTaskWoken);
}

void AmpButton::init(TouchTaskListener *listener) {
  _listener = listener;
  AmpButton::buttonEventQueue = xQueueCreate(10, sizeof(uint32_t));

  // configure input button
  gpio_config_t io_config;
  io_config.intr_type = GPIO_INTR_ANYEDGE;
  io_config.mode = GPIO_MODE_INPUT;
  io_config.pull_down_en = GPIO_PULLDOWN_DISABLE;
  io_config.pull_up_en = GPIO_PULLUP_DISABLE;

  io_config.pin_bit_mask = 0;
  io_config.pin_bit_mask = IO_PIN_SELECT(BUTTON_INPUT);
  gpio_config(&io_config);

  // hold off until the user has released the button
  while (!gpio_get_level(BUTTON_INPUT)) {
    delay(10);
  }

  auto ret = gpio_install_isr_service(ESP_INTR_FLAG_LEVEL1);
  ESP_ERROR_CHECK(ret);
  ret = gpio_isr_handler_add(BUTTON_INPUT, button_isr_handler, (void*) BUTTON_INPUT);
  ESP_ERROR_CHECK(ret);
}

void AmpButton::deinit() {
  goingDown = true;
  //detachInterrupt(digitalPinToInterrupt(BUTTON_INPUT));
}

void AmpButton::process() {
  uint32_t button;
  // process input events from queue
  if (xQueueReceive(AmpButton::buttonEventQueue, &button, 0)) {
    auto level = gpio_get_level(BUTTON_INPUT);
    if (_listener != nullptr)
      level ? _listener->onTouchUp() : _listener->onTouchDown();
  }
}