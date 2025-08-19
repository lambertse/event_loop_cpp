#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "event_loop/poll.h"
#include "event_loop/shared/logging.h"

bool Poll::init(int fd) {
  struct epoll_event ev;
  _poll_fd = epoll_create1(0);
  if (_poll_fd == -1) {
    LOG_INFO("epoll_create1 failed: " + std::to_string(errno));
    return false;
  }

  ev.events = EPOLLIN;
  ev.data.fd = fd;
  if (epoll_ctl(_poll_fd, EPOLL_CTL_ADD, fd, &ev) == -1) {
    LOG_INFO("epoll_ctl: listen_sock");
    return false;
  }

  return true;
}

bool Poll::start() {
  struct epoll_event events[MAX_EVENTS];
  while (!_is_stopped) {
    auto nfds = epoll_wait(_poll_fd, events, MAX_EVENTS, -1);
    for (int n = 0; n < nfds; ++n) {
      auto fd = events[n].data.fd;
      if (_conn_io_cb) {
        _conn_io_cb(fd);
      }
    }
  }
  return true;
}

bool Poll::stop() {
  if (_poll_fd >= 0) {
    close(_poll_fd);
    _poll_fd = -1;
  }
  _is_stopped = true;
  return true;
}

void Poll::add_connection(int fd) {
  // Add a new connection to the poll
  epoll_event ev;
  ev.events = EPOLLIN | EPOLLET;
  ev.data.fd = fd;
  if (epoll_ctl(_poll_fd, EPOLL_CTL_ADD, fd, &ev) == -1) {
    LOG_INFO("epoll_ctl: conn_sock");
    exit(EXIT_FAILURE);
  }
}

void Poll::remove_connection(int fd) {
  // Remove a connection from the poll
}

void Poll::set_connection_io_callback(ConnectionIOCallback cb) {
  _conn_io_cb = std::move(cb);
}
