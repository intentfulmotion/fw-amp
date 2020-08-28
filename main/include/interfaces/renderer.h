#pragma once
#include <interfaces/command-listener.h>
#include <hal/lights.h>

class Renderer : public CommandListener {
  protected:
    Lights* _lights;
    LightCommand _headlightCommand = LightCommand::LightsRunning;
    LightCommand _brakelightCommand = LightCommand::LightsRunning;
    LightCommand _turnlightCommand = LightCommand::LightsTurnCenter;

  public:
    Renderer(Lights *lights) { _lights = lights; }

    LightCommand getBrakeCommand() { return _brakelightCommand; }
    LightCommand getHeadlightCommand() { return _headlightCommand; }
    LightCommand getTurnLightCommand() { return _turnlightCommand; }

    virtual void setBrakes(LightCommand command);
    virtual void setTurnLights(LightCommand command);
    virtual void setHeadlight(LightCommand command);

    virtual void process() = 0;
    virtual void shutdown();
};

class LoopingRenderer : public Renderer {
  public:
    LoopingRenderer(Lights *lights) : Renderer(lights) { }

    virtual void process() = 0;
    bool hasProcessLoop() { return true; }

    void setBrakes(LightCommand command) {}
    void setTurnLights(LightCommand command) {}
    void setHeadlight(LightCommand command) {}

    void shutdown() {  }
};