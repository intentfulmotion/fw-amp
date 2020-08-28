#include <hal/amp-1.0.0/amp-power.h>

void AmpPower::init() {
  gpio_config_t io_config;

  // power hold
  io_config.intr_type = GPIO_INTR_DISABLE;
  io_config.mode = GPIO_MODE_INPUT_OUTPUT;
  io_config.pull_down_en = GPIO_PULLDOWN_DISABLE;
  io_config.pull_up_en = GPIO_PULLUP_DISABLE;
  io_config.pin_bit_mask = 0;
  io_config.pin_bit_mask = IO_PIN_SELECT(POWER_HOLD);
  auto ret = gpio_config(&io_config);
  ESP_ERROR_CHECK(ret);

  // input pin setup w/o interrupt
  io_config.intr_type = GPIO_INTR_DISABLE;
  io_config.mode = GPIO_MODE_INPUT;
  io_config.pull_down_en = GPIO_PULLDOWN_DISABLE;
  io_config.pull_up_en = GPIO_PULLUP_DISABLE;

  io_config.pin_bit_mask = 0;
  io_config.pin_bit_mask = 
      IO_PIN_SELECT(VBAT_SENSE) |
      IO_PIN_SELECT(BAT_CHRG)   |
      IO_PIN_SELECT(BAT_DONE);
  ret = gpio_config(&io_config);
  ESP_ERROR_CHECK(ret);

  // set the power hold on the STM6601 power supervisor
  gpio_set_level(POWER_HOLD, 1);
  gpio_hold_en(POWER_HOLD);

  adc1_config_width(ADC_WIDTH_BIT_12);
  adc1_config_channel_atten(ADC1_CHANNEL_3, ADC_ATTEN_DB_0);
}

void AmpPower::process() {
  // TODO: add pull up resistor to done and charge inputs
  charging = !gpio_get_level(BAT_CHRG);
  // done = !gpio_get_level(BAT_DONE);

  batteryAdcReading = adc1_get_raw(ADC1_CHANNEL_3);
  batteryReading = (float)batteryAdcReading / 500.f;
  batteryReading = std::min(batteryReading, 4.2f);
  done = batteryReading >= 4.15f;
  
  batteryPresent = batteryReading >= 2.5;
  batteryLevel = percentageFromReading(batteryReading);
}

void AmpPower::deinit() {
  // release the power hold on the STM6601 power supervisor
  gpio_hold_dis(POWER_HOLD);
  gpio_set_level(POWER_HOLD, 0);
}