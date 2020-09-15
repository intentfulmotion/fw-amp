# Amp Firmware

Firmware powering the Amp smart lighting controller. Built on ESP-IDF 4.1.

## Features
* Automatic brake lights
* Motion or gesture controlled turn indicators
* Lighting and motion detection customizable via the Amp Mixer mobile app
* Remote control via the Amp Mixer app or Bluetooth LE
* 5 alternative lighting modes (rainbows, theater chase lights, etc.)

## Specs
* ESP32 WROOM32E dual core Xtensa processor running at 80 Mhz (can be clocked up to 240 Mhz)
* LIS3DH12 high performance, 3 axis accelerometer
* Up to 2A of regulated 3.3V power from a 2000 mAh Li-Po battery
* USB-C charging
* Serial port + programming over USB-C
* 4 [Qwiic](https://www.sparkfun.com/qwiic) compatible I/O ports that can also be used as lighting output to
  * 8 channels of WS2812B (NeoPixels)
  * 4 channels of APA102 (DotStar)

## Dependencies

* [esp-nimble-cpp](https://github.com/intentfulmotion/esp-nimble-cpp) - modified version of https://github.com/h2zero/esp-nimble-cpp
* [FastLED-idf](https://github.com/intentfulmotion/FastLED-idf) - modified version of https://github.com/bbulkow/FastLED-idf
* [lis3dh-esp-idf](https://github.com/intentfulmotion/lis3dh-esp-idf) - modified version of https://github.com/gschorcht/lis3dh-esp-idf

## Build from source

1. `idf.py menuconfig` and configure the IDF project. This project uses the NimBLE stack instead of Bluedroid
2. `idf.py build` to build the project
3. `idf.py flash -p <port>` to flash it to an Amp / ESP32
