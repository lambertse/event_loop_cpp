#pragma once
#include <cstdint>
#include <map>
#include <memory>

constexpr int k_max_msg = 256;

enum ConnectionState {
  REQUEST = 0,
  RESPONSE,
  END,
};

struct Connection {
  int fd;
  ConnectionState state;
  // Buffer for reading
  size_t rbuf_size = 0;
  uint8_t rbuf[4 + k_max_msg];
  // Buffer for wriring
  size_t wbuf_size = 0;
  size_t wbuf_sent = 0;
  uint8_t wbuf[4 + k_max_msg];
};
using ConnectionSharedPtr = std::shared_ptr<Connection>;
using FDConnectionMap = std::map<int, ConnectionSharedPtr>;
