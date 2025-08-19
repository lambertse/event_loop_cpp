#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "../poll.h"
#include "shared/logging.h"
#include "shared/utils.h"

bool Poll::init() {
  struct epoll_event ev;
  poll_fd = epoll_create1(0);
  if (poll_fd == -1) {
    LOG_INFO("epoll_create1 failed: " + std::to_string(errno));
    return false;
  }

  ev.events = EPOLLIN;
  ev.data.fd = server_fd;
  if (epoll_ctl(poll_fd, EPOLL_CTL_ADD, server_fd, &ev) == -1) {
    LOG_INFO("epoll_ctl: listen_sock");
    return false;
  }

  return true;
}

bool Poll::start() {
  struct epoll_event events[MAX_EVENTS];
  while (true) {
    auto nfds = epoll_wait(poll_fd, events, MAX_EVENTS, -1);
    for (int n = 0; n < nfds; ++n) {
      if (events[n].data.fd == server_fd) {
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

bool Poll::stop() {}

void Poll::add_connection(int fd) {
  // Add a new connection to the poll
  epoll_event ev;
  ev.events = EPOLLIN | EPOLLET;
  ev.data.fd = fd;
  if (epoll_ctl(poll_fd, EPOLL_CTL_ADD, fd, &ev) == -1) {
    LOG_INFO("epoll_ctl: conn_sock");
    exit(EXIT_FAILURE);
  }
}

void Poll::remove_connection(int fd) {
  // Remove a connection from the poll
}

void Poll::accept_new_conn() {
  socklen_t addrlen = sizeof(addr);
  auto connFD = accept(server_fd, (struct sockaddr *)&addr, &addrlen);
  if (connFD == -1) {
    LOG_INFO("accept failed: " + std::to_string(errno));
    exit(EXIT_FAILURE);
  }
  utils::set_fb_nonblocking(connFD);
  _poll(connFD);

  LOG_DEBUG("accept_new_conn with fd " + std::to_string(connFD));
  ConnectionSharedPtr newConn = std::make_shared<Connection>();
  newConn->fd = connFD;
  newConn->state = ConnectionState::REQUEST;

  fd_2_con[connFD] = newConn;
}
