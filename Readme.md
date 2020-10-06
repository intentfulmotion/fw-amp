# Amp Firmware

Firmware powering the Amp smart lighting controller. Built on a slightly modified [ESP-IDF 4.1](https://github.com/intentfulmotion/esp-idf).

## Features

* Automatic brake lights
* Motion or gesture controlled turn indicators
* Lighting and motion detection customizable via the Amp Mixer mobile app
* Remote control via the Amp Mixer app or Bluetooth LE
* Customizable lighting effects that react to actions (turning, braking, etc.)
* [Well documented APIs](https://docs.ridewithamp.com) for modifying your Amp

## Specs

* ESP32 WROOM32E dual core Xtensa processor running at 80 Mhz (can be clocked up to 240 Mhz)
* LIS3DH12 high performance, 3 axis accelerometer
* Up to 2A of regulated 3.3V power from a 2000 mAh Li-Po battery
* USB-C charging
* Serial port + programming over USB-C
* 4 [Qwiic](https://www.sparkfun.com/qwiic) compatible I/O ports that can also be used as lighting output to
  * 8 channels of WS2812/WS2812B/WS2813/SK6812 (NeoPixels)
  * 4 channels of APA102 (DotStar)

## Dependencies

* [esp-nimble-cpp](https://github.com/intentfulmotion/esp-nimble-cpp)
* [AddressableLED](https://github.com/intentfulmotion/AddressableLED)
* [lis3dh-esp-idf](https://github.com/intentfulmotion/lis3dh-esp-idf)

## Build from source

1. `idf.py menuconfig` and configure the IDF project. This project uses the NimBLE stack instead of Bluedroid
2. `idf.py build` to build the project
3. `idf.py flash -p <port>` to flash it to an Amp / ESP32

## Credits

Parts of this software include derivations of other open source software. A full list is available below:

* [WS2812FX](https://github.com/kitesurfer1404/WS2812FX) by Harm Aldick
* [SmartLeds](https://github.com/RoboticsBrno/SmartLeds) by RoboticsBrno
* [esp-nimble-cpp](https://github.com/h2zero/esp-nimble-cpp) by h2zero
* [lis3dh-esp-idf](https://github.com/gschorcht/lis3dh-esp-idf) by Gunar Schorcht
* [esp-idf](https://github.com/espressif/esp-idf) by Espressif