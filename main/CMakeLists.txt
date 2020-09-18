idf_component_register(
  SRCS  
    "src/hal/amp-1.0.0/light-controller.cpp"
    "src/hal/amp-1.0.0/amp-button.cpp"
    "src/hal/amp-1.0.0/amp-imu.cpp"
    "src/hal/amp-1.0.0/amp-leds.cpp"
    "src/hal/amp-1.0.0/amp-power.cpp"
    "src/hal/amp-1.0.0/amp-storage.cpp"
    "src/hal/ble.cpp"
    "src/hal/buttons.cpp"
    "src/hal/config.cpp"
    "src/hal/lights.cpp"
    "src/hal/motion.cpp"
    "src/hal/power.cpp"
    "src/hal/updater.cpp"
    "src/renderers/pattern.cpp"
    "src/renderers/running.cpp"
    "src/services/battery-service.cpp"
    "src/services/config-service.cpp"
    "src/services/device-info-service.cpp"
    "src/services/vehicle-service.cpp"
    "src/services/update-service.cpp"
    "src/app.cpp"
    "src/amp.cpp"
    "src/constants.cpp"
    "src/log.cpp"
    "src/main.cpp"

  INCLUDE_DIRS 
    "include/" 
    "include/hal/"
    "include/hal/amp-1.0.0/"
    "include/interfaces/"
    "include/models/"
    "include/renderers/"
    "include/services/"
    "libraries/ArduinoJSON/"

  REQUIRES esp-arduino-shim lis3dh bt spi_flash soc efuse nvs_flash esp-nimble-cpp esp_adc_cal
  PRIV_REQUIRES app_update fatfs freertos spiffs soc efuse NeoPixelBus-idf
)
