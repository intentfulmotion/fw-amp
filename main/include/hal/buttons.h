#pragma once
#include <common.h>
#include <models/touch-type.h>
#include <interfaces/lifecycle.h>
#include <interfaces/touch-listener.h>

#if defined(AMP_1_0_x)
  #include <hal/amp-1.0.0/amp-button.h>
#endif

#include <hal/power.h>

#include <memory>

static const char* BUTTONS_TAG = "buttons";

class Buttons : public LifecycleBase, public TouchTaskListener {
  std::vector<TouchListener*> touchListeners;
  std::vector<TouchType> touches;
  AmpButton ampButtons;
  TaskHandle_t inputTaskHandle;

  long touchEventTimeout = 500;
  long holdDuration = 500;
  long touchStart, touchEnd;

  public:
    void onPowerUp();
    void onPowerDown();
    void process();

    void onTouchDown();
    void onTouchUp();

    void addTouchListener(TouchListener *listener);

    static void inputTask(void* params);
};