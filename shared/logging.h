#pragma once
#include <iostream>
#include <string>

namespace logging {
enum LogLevel {
  DEBUG = 0,
  INFO = 1,
};

static LogLevel log_level = INFO;

static void debug(const std::string &str) {
  if (log_level ^ DEBUG) {
    return;
  }
  std::cout << "DEBUG: " << str << std::endl;
}
static void info(const std::string &str) {
  if (log_level ^ INFO) {
    return;
  }
  std::cout << "INFO: " << str << std::endl;
}

} // namespace logging
#define LOG_DEBUG(str) logging::debug(str)
#define LOG_INFO(str) logging::info(str)
