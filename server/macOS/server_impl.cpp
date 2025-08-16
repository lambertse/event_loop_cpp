#include <sys/event.h>
#include <sys/types.h>

#include "../server_impl.h"

bool ServerImpl::_init() {
  pollFD = kqueue();
  if (pollFD == -1) {
    LOG_INFO("kqueue failed: " + std::to_string(errno));
    return false;
  }

  struct kevent ev;
  EV_SET(&ev, serverFD, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, NULL);
  if (kevent(pollFD, &ev, 1, NULL, 0, NULL) == -1) {
    LOG_INFO("kevent: listen_sock failed: " + std::to_string(errno));
    return false;
  }
  return true;
}

bool ServerImpl::_start() {
  struct kevent events[MAX_EVENTS];
  while (true) {
    auto nfds = kevent(pollFD, NULL, 0, events, MAX_EVENTS, NULL);
    if (nfds == -1) {
      LOG_INFO("kevent failed: " + std::to_string(errno));
      break;
    }

    for (int n = 0; n < nfds; ++n) {
      int fd = static_cast<int>(events[n].ident);

      if (fd == serverFD) {
        accept_new_conn();
      } else {
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
  return true;
}

void ServerImpl::_poll(const int &connFD) {
  struct kevent ev;
  EV_SET(&ev, connFD, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, NULL);
  if (kevent(pollFD, &ev, 1, NULL, 0, NULL) == -1) {
    LOG_INFO("kevent: conn_sock failed: " + std::to_string(errno));
    exit(EXIT_FAILURE);
  }
}
