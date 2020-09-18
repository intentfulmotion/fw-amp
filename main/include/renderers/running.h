#pragma once
#include "FreeRTOS.h"
#include <interfaces/renderer.h>
#include <models/config.h>

class RunningRenderer : public Renderer {
  AmpConfig *_config;
  TaskHandle_t brakeHandle = nullptr;
  TaskHandle_t turnHandle = nullptr;

  FreeRTOS::Semaphore brakeInit;
  FreeRTOS::Semaphore turnInit;

  ColorRGB brightHeadlight = ColorRGB(255, 255, 255);
  ColorRGB runningHeadlight = ColorRGB(127, 127, 127);
  ColorRGB brightBrake = ColorRGB(255, 0, 0);
  ColorRGB runningBrake = ColorRGB(127, 0, 0);
  ColorRGB turn = ColorRGB(127, 127, 0);

  ColorRGBW extendedBrightHeadlight = ColorRGBW(0, 0, 0, 255);
  ColorRGBW extendedRunningHeadlight = ColorRGBW(0, 0, 0, 127);

  public:
    RunningRenderer(Lights *lights, AmpConfig *config);
    void process();
    bool hasProcessLoop() { return true; }
    void shutdown();

    void setBrakes(LightCommand command);
    void setTurnLights(LightCommand command);
    void setHeadlight(LightCommand command);

    static void brakeFunction(void *args);
    static void turnFunction(void *args);
};