#pragma once

#include "define.h"

class ConnectionManager {
 public:
  explicit ConnectionManager();
  virtual ~ConnectionManager() = default;

  void remove_connection(int fd);
  void handle_connection_io(ConnectionSharedPtr conn);

 private:
  bool try_one_request(ConnectionSharedPtr conn);
  void state_request(ConnectionSharedPtr conn);
  void state_response(ConnectionSharedPtr conn);
};
