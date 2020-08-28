#include <log.h>

std::string Log::_prefix;
std::string Log::_suffix;
FreeRTOS::Semaphore Log::logSemaphore;