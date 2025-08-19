#pragma once
#include <functional>

#include "define.h"
#include "protobuf_handler.h"

using MessageHandler =
    std::function<std::vector<uint8_t>(const std::vector<uint8_t>&)>;

class ConnectionManager {
 public:
  explicit ConnectionManager();
  virtual ~ConnectionManager() = default;

  void remove_connection(int fd);
  void handle_connection_io(ConnectionSharedPtr conn);
  void set_message_handler(MessageHandler handler);

 private:
  bool try_one_request(ConnectionSharedPtr conn);
  void state_request(ConnectionSharedPtr conn);
  void state_response(ConnectionSharedPtr conn);

  std::unique_ptr<ProtobufHandler> _protocol_handler;

  MessageHandler _message_handler;
};
