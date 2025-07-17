#pragma once

#include <memory>

class Server {
public:
  Server();
  ~Server();

  bool init();
  bool start();
  bool stop();
  bool deinit();

private:
  std::shared_ptr<class ServerImpl> _impl;
};
