#include <board.h>

void Board::setupBoard() {
  gpio_config_t io_config;

  // led pins setup
  io_config.intr_type = GPIO_INTR_DISABLE;
  io_config.mode = GPIO_MODE_INPUT_OUTPUT;
  io_config.pull_down_en = GPIO_PULLDOWN_DISABLE;
  io_config.pull_up_en = GPIO_PULLUP_DISABLE;
  io_config.pin_bit_mask = 0;

  io_config.pin_bit_mask = 
    IO_PIN_SELECT(LED_STATUS)         |
    IO_PIN_SELECT(LED_CHANNEL_1_DATA) | 
    IO_PIN_SELECT(LED_CHANNEL_1_CLK)  |
    IO_PIN_SELECT(LED_CHANNEL_2_DATA) | 
    IO_PIN_SELECT(LED_CHANNEL_2_CLK)  |
    IO_PIN_SELECT(LED_CHANNEL_3_DATA) | 
    IO_PIN_SELECT(LED_CHANNEL_3_CLK)  |
    IO_PIN_SELECT(LED_CHANNEL_4_DATA) | 
    IO_PIN_SELECT(LED_CHANNEL_4_CLK);
  auto ret = gpio_config(&io_config);
  ESP_ERROR_CHECK(ret);

  // power hold
  io_config.intr_type = GPIO_INTR_DISABLE;
  io_config.mode = GPIO_MODE_INPUT_OUTPUT;
  io_config.pull_down_en = GPIO_PULLDOWN_DISABLE;
  io_config.pull_up_en = GPIO_PULLUP_DISABLE;
  io_config.pin_bit_mask = 0;
  io_config.pin_bit_mask = IO_PIN_SELECT(POWER_HOLD);
  ret = gpio_config(&io_config);
  ESP_ERROR_CHECK(ret);

  // input pin setup w/ interrupt
  io_config.intr_type = GPIO_INTR_ANYEDGE;
  io_config.mode = GPIO_MODE_INPUT;
  io_config.pull_down_en = GPIO_PULLDOWN_DISABLE;
  io_config.pull_up_en = GPIO_PULLUP_DISABLE;

  io_config.pin_bit_mask = 0;
  io_config.pin_bit_mask = 
    IO_PIN_SELECT(AMP_INPUT);
  ret = gpio_config(&io_config);

  // input pin setup w/ interrupt
  io_config.intr_type = GPIO_INTR_DISABLE;
  io_config.mode = GPIO_MODE_INPUT;
  io_config.pull_down_en = GPIO_PULLDOWN_DISABLE;
  io_config.pull_up_en = GPIO_PULLUP_DISABLE;

  io_config.pin_bit_mask = 0;
  io_config.pin_bit_mask = 
    IO_PIN_SELECT(VBAT_SENSE);
  ret = gpio_config(&io_config);
  ESP_ERROR_CHECK(ret);
}