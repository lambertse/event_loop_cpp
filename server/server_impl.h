#pragma once

#include <netinet/ip.h>

#include <memory>

#include "config.h"
#include "connection_manager.h"
#include "define.h"

class ServerImpl {
 public:
  ServerImpl(ServerConfig config = ServerConfig::default_config());
  ~ServerImpl();

  bool init();
  bool start();
  bool stop();
  bool deinit();

  void set_message_handler(MessageHandler handler);

 private:
  void add_conection(int fd);
  void accept_new_connection();
  void handle_connection_io(int fd);
  bool setup_socket();

 private:
  int _server_fd;
  ServerConfig _config;
  struct sockaddr_in _addr;
  FDConnectionMap _fd_2_conn;
  std::unique_ptr<class Poll> _poll;
  std::unique_ptr<ConnectionManager> _connection_manager;
};
