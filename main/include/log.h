#pragma once
#include <esp_log.h>
#include <stdarg.h>
#include <string>
#include "FreeRTOS.h"

#define AMP_LOG_LEVEL_SILENT    0
#define AMP_LOG_LEVEL_FATAL     1
#define AMP_LOG_LEVEL_ERROR     2
#define AMP_LOG_LEVEL_WARNING   3
#define AMP_LOG_LEVEL_NOTICE    4
#define AMP_LOG_LEVEL_TRACE     5
#define AMP_LOG_LEVEL_VERBOSE   6

#if !defined(AMP_LOG_LEVEL)
  #define AMP_LOG_LEVEL AMP_LOG_LEVEL_SILENT
#endif

#define COLOR_RESET         "\033[0m"
#define COLOR_RED           "\033[0;31m"
#define COLOR_GREEN         "\033[0;32m"
#define COLOR_YELLOW        "\033[0;33m"
#define COLOR_BLUE          "\033[0;34m"
#define COLOR_MAGENTA       "\033[0;35m"
#define COLOR_CYAN          "\033[0;36m"
#define COLOR_RED_BOLD      "\033[1;31m"
#define COLOR_GREEN_BOLD    "\033[1;32m"
#define COLOR_YELLOW_BOLD   "\033[1;33m"
#define COLOR_BLUE_BOLD     "\033[1;34m"
#define COLOR_MAGENTA_BOLD  "\033[1;35m"
#define COLOR_CYAN_BOLD     "\033[1;36m"

class Log {
  static std::string _prefix;
  static std::string _suffix;
  static FreeRTOS::Semaphore logSemaphore;

  template<typename... Args> static void log(std::string format, Args... args) {
    logSemaphore.wait("log");
    logSemaphore.take("log");

    // prefix
    printf(_prefix.c_str());

    // format
    printf(format.c_str(), args...);

    // suffix
    printf(_suffix.c_str());

    logSemaphore.give();
  }

  public:
    static void setPrefix(std::string prefix) { _prefix = prefix; }
    static void setSuffix(std::string suffix) { _suffix = suffix; }
    
    template<typename... Args> static void fatal(std::string format, Args... args) {
#if AMP_LOG_LEVEL >= AMP_LOG_LEVEL_FATAL
      printf("%s", COLOR_RED_BOLD);
      log(format, args...);
      printf("%s", COLOR_RESET);
#endif
    }

    template<typename... Args> static void error(std::string format, Args... args) {
#if AMP_LOG_LEVEL >= AMP_LOG_LEVEL_ERROR
      printf("%s", COLOR_RED);
      log(format, args...);
      printf("%s", COLOR_RESET);
#endif
    }

   template<typename... Args>  static void warning(std::string format, Args... args) {
#if AMP_LOG_LEVEL >= AMP_LOG_LEVEL_WARNING
      printf("%s", COLOR_YELLOW);
      log(format, args...);
      printf("%s", COLOR_RESET);
#endif
    }

    template<typename... Args> static void notice(std::string format, Args... args) {
#if AMP_LOG_LEVEL >= AMP_LOG_LEVEL_NOTICE
      log(format, args...);
#endif
    }

   template<typename... Args>  static void trace(std::string format, Args... args) {
#if AMP_LOG_LEVEL >= AMP_LOG_LEVEL_TRACE
      log(format, args...);
#endif
    }

    template<typename... Args> static void verbose(std::string format, Args... args) {
#if AMP_LOG_LEVEL >= AMP_LOG_LEVEL_VERBOSE
      log(format, args...);
#endif
    }
};