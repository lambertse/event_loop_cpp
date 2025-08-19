#pragma once

#include <functional>

using ConnectionIOCallback = std::function<void(int fd)>;

constexpr int MAX_EVENTS = 10;

class Poll {
 public:
  Poll() = default;
  virtual ~Poll() = default;
  virtual bool init(int fd);
  virtual bool start();
  virtual bool stop();
  virtual void add_connection(int fd);
  virtual void remove_connection(int fd);

  void set_connection_io_callback(ConnectionIOCallback cb);

 protected:
  ConnectionIOCallback _conn_io_cb;
  int _poll_fd = -1;
  bool _is_stopped = false;
};
