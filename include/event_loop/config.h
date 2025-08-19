#pragma once
#include <string>

struct ServerConfig {
  int port;
  int max_connections;
  int max_message_size;
  bool enable_logging;
  std::string log_level;

  static ServerConfig from_file(const std::string& config_path) {}
  static ServerConfig default_config() {
    static ServerConfig config;
    config.port = 8100;
    config.max_connections = 100;
    config.max_message_size = 4096;
    config.enable_logging = true;
    config.log_level = "INFO";
    return config;
  }
};
