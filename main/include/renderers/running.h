#pragma once
#include "FreeRTOS.h"
#include <interfaces/renderer.h>
#include <models/config.h>

static const char* RUNNING_TAG = "running-lights";

class RunningRenderer : public Renderer {
  AmpConfig *_config;
  TaskHandle_t brakeHandle = nullptr;
  TaskHandle_t turnHandle = nullptr;

  FreeRTOS::Semaphore brakeInit;
  FreeRTOS::Semaphore turnInit;

  Color brightHeadlight = Color(255, 255, 255);
  Color runningHeadlight = Color(127, 127, 127);
  Color brightBrake = Color(255, 0, 0);
  Color runningBrake = Color(127, 0, 0);
  Color turn = Color(127, 127, 0);

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