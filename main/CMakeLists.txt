idf_component_register(
  SRCS    
    "src/hal/common/amp-leds.cpp"
    "src/hal/common/amp-storage.cpp"
    "src/interfaces/motion-provider.cpp"
    "src/interfaces/power-provider.cpp"
    "src/hal/ble.cpp"
    "src/hal/buttons.cpp"
    "src/hal/vesc-can.cpp"
    "src/hal/config.cpp"
    "src/hal/lights.cpp"
    "src/hal/internal-motion.cpp"
    "src/hal/power.cpp"
    "src/hal/updater.cpp"
    "src/services/battery-service.cpp"
    "src/services/config-service.cpp"
    "src/services/device-info-service.cpp"
    "src/services/vehicle-service.cpp"
    "src/services/update-service.cpp"
    "src/services/vesc-service.cpp"
    "src/app.cpp"
    "src/amp.cpp"
    "src/constants.cpp"
    "src/main.cpp"

  INCLUDE_DIRS 
    "include/" 
    "include/hal/"
    "include/hal/common/"
    "include/hal/pint/"
    "include/interfaces/"
    "include/models/"
    "include/services/"
    "libraries/ArduinoJson/"

  REQUIRES freertos lis3dh bt spi_flash soc efuse nvs_flash esp_adc_cal AddressableLED
  PRIV_REQUIRES app_update fatfs freertos spiffs soc efuse esp-nimble-cpp
)

target_compile_options(${COMPONENT_LIB} PRIVATE -std=c++17 -Wswitch)