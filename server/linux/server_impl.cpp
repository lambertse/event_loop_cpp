#include <sys/epoll.h>

#include "../server_impl.h"

bool ServerImpl::_init() {
  struct epoll_event ev;
  pollFD = epoll_create1(0);
  if (pollFD == -1) {
    LOG_INFO("epoll_create1 failed: " + std::to_string(errno));
    return false;
  }

  ev.events = EPOLLIN;
  ev.data.fd = serverFD;
  if (epoll_ctl(pollFD, EPOLL_CTL_ADD, serverFD, &ev) == -1) {
    LOG_INFO("epoll_ctl: listen_sock");
    return false;
  }

  return true;
}

bool ServerImpl::_start() {
  struct epoll_event events[MAX_EVENTS];
  while (true) {
    auto nfds = epoll_wait(pollFD, events, MAX_EVENTS, -1);
    for (int n = 0; n < nfds; ++n) {
      if (events[n].data.fd == serverFD) {
        accept_new_conn();
      } else {
        int fd = events[n].data.fd;
        if (fd2Conn.count(fd) <= 0) {
          LOG_INFO("Connection not found for fd: " + std::to_string(fd));
          continue;
        }
        ConnectionSharedPtr conn = fd2Conn[fd];

        if (conn->state == ConnectionState::END) {
          LOG_DEBUG("Connection ended for fd: " + std::to_string(fd));
          close(fd);
          fd2Conn.erase(fd);
          continue;
        }

        connection_io(conn);
      }
    }
  }
}

void ServerImpl::_poll(const int &connFD) {
  epoll_event ev;
  ev.events = EPOLLIN | EPOLLET;
  ev.data.fd = connFD;
  if (epoll_ctl(epollFD, EPOLL_CTL_ADD, connFD, &ev) == -1) {
    LOG_INFO("epoll_ctl: conn_sock");
    exit(EXIT_FAILURE);
  }
}
