#include <hal/power.h>

FreeRTOS::Semaphore Power::powerDown = FreeRTOS::Semaphore("power");

Power::Power() {
  touchEventQueue = xQueueCreate(10, sizeof(std::vector<TouchType>));
}

void Power::onPowerUp() {
  ampPower.init();

  for (auto listener : lifecycleListeners)
    listener->onPowerUp();

  ampPower.process();
  status = calculatePowerStatus(ampPower.batteryPresent, ampPower.charging, ampPower.done, ampPower.batteryLevel);

  notifyPowerListeners();
}

void Power::onPowerDown() {
  ESP_LOGD(POWER_TAG,"Powering down");

  if (restartNext)
    esp_restart();
  else
    ampPower.deinit();
}

void Power::process() {
  if (uxQueueMessagesWaiting(touchEventQueue)) {
    xQueueReceive(touchEventQueue, &touches, 0);

    onTouchEvent(&touches);
  }
  
  PowerStatus newStatus;
  ampPower.process();
  newStatus = calculatePowerStatus(ampPower.batteryPresent, ampPower.charging, ampPower.done, ampPower.batteryLevel);

  // if the power status has changed, notify our listeners
  if (status != newStatus) {
    status = newStatus;
    notifyPowerListeners();
  }
}

void Power::addPowerLevelListener(PowerListener *listener) {
  powerLevelListeners.push_back(listener);
}

void Power::addLifecycleListener(LifecycleBase *listener) {
  lifecycleListeners.push_back(listener);
}

void Power::notifyPowerListeners() {
  ESP_LOGV(POWER_TAG,"Power status notification: Charging: %s, Level: %d (%d %%), Battery Present: %s", status.charging ? "true" : "false", status.level, status.percentage, status.batteryPresent ? "true" : "false");

  for(auto listener : powerLevelListeners)
    if (listener->powerStatusQueue != NULL)
      xQueueSend(listener->powerStatusQueue, &status, 0);
}

PowerStatus Power::calculatePowerStatus(bool batteryPresent, bool charging, bool done, uint8_t batteryLevel) {
  PowerStatus ns;
  ns.batteryPresent = batteryPresent;
  ns.charging = charging;
  ns.doneCharging = done;
  ns.percentage = batteryLevel;

  if (!batteryPresent) {
    ns.percentage = 100;
    ns.level = PowerLevel::Normal;
  }
  else if (ns.doneCharging)
    ns.level = PowerLevel::Charged;
  else if (batteryLevel < BATTERY_LOW)
    ns.level = PowerLevel::Critical;
  else if (batteryLevel < BATTERY_NORMAL)
    ns.level = PowerLevel::Low;
  else
    ns.level = PowerLevel::Normal;

  return ns;
}

void Power::onTouchEvent(std::vector<TouchType> *touches) {
  if (touches->size() == 1) {
    if ((*touches)[0] == TouchType::Hold) {
      shutdown();
    }
  }
}

void Power::shutdown(bool restart) {
  powerDown.take("power");
  restartNext = restart;

  for (auto listener = lifecycleListeners.rbegin(); listener != lifecycleListeners.rend(); ++listener)
    (*listener)->onPowerDown();

  onPowerDown();
}