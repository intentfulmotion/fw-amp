#include <interfaces/power-provider.h>

FreeRTOS::Semaphore PowerProvider::powerDown = FreeRTOS::Semaphore("power");

void PowerProvider::addPowerLevelListener(PowerListener *listener)
{
  powerLevelListeners.push_back(listener);
}

void PowerProvider::addLifecycleListener(LifecycleBase *listener)
{
  lifecycleListeners.push_back(listener);
}

void PowerProvider::notifyPowerListeners()
{
  ESP_LOGV(POWER_TAG, "Power status notification: Charging: %s, Level: %d (%d %%), Battery Present: %s", powerStatus.charging ? "true" : "false", powerStatus.level, powerStatus.percentage, powerStatus.batteryPresent ? "true" : "false");

  for (auto listener : powerLevelListeners)
    if (listener->powerStatusQueue != NULL)
      xQueueSendToBack(listener->powerStatusQueue, &powerStatus, 0);
    else
      ESP_LOGW(POWER_TAG, "Can't notify listener because of null queue");
}

PowerStatus PowerProvider::calculatePowerStatus(bool batteryPresent, bool charging, bool done, uint8_t batteryLevel)
{
  PowerStatus ns;
  ns.batteryPresent = batteryPresent;
  ns.charging = charging;
  ns.doneCharging = done;
  ns.percentage = batteryLevel;

  if (!batteryPresent)
  {
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