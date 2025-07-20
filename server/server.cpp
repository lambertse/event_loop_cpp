#include "server.h"
#include "server_define.h"
#include "shared/logging.h"
#include "shared/utils.h"
#include <csignal>

#include <cstddef>
#include <cstring>
#include <map>
#include <memory>
#include <netinet/ip.h>
#include <poll.h>
#include <string>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <vector>

constexpr int MAX_EVENTS = 10;

class ServerImpl {
public:
  ServerImpl() {}
  ~ServerImpl() {}
  bool init(int port) {
    serverFD = socket(AF_INET, SOCK_STREAM, 0);
    int val = false;
    setsockopt(serverFD, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));
    if (serverFD < 0) {
      LOG_DEBUG("fd is not valid " + std::to_string(serverFD));
      return false;
    }
    utils::set_fb_nonblocking(serverFD);

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(0);

    if (bind(serverFD, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
      LOG_DEBUG("bind failed");
      return false;
    }
    if (listen(serverFD, 5) < 0) {
      LOG_DEBUG("listen failed");
      return false;
    }
    LOG_INFO("Server is listening on port " + std::to_string(port));

    struct epoll_event ev;
    epollFD = epoll_create1(0);
    if (epollFD == -1) {
      LOG_INFO("epoll_create1 failed: " + std::to_string(errno));
      return false;
    }

    ev.events = EPOLLIN;
    ev.data.fd = serverFD;
    if (epoll_ctl(epollFD, EPOLL_CTL_ADD, serverFD, &ev) == -1) {
      LOG_INFO("epoll_ctl: listen_sock");
      return false;
    }

    return true;
  }

  bool start() {
    LOG_INFO("Server started with fd " + std::to_string(serverFD));
    int nfds;
    struct epoll_event events[MAX_EVENTS];
    while (true) {
      nfds = epoll_wait(epollFD, events, MAX_EVENTS, -1);
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

  bool stop() {
    // Close all connections
    for (auto &[fd, con] : fd2Conn) {
      if (con) {
        (void)close(fd);
      }
    }
    fd2Conn.clear();
    (void)close(serverFD);
    return true;
  }

  bool deinit() {
    LOG_DEBUG("Deinitializing server");
    stop();
    return true;
  }

  void connection_io(ConnectionSharedPtr conn) {
    if (conn->state == ConnectionState::REQUEST) {
      state_request(conn);
    } else if (conn->state == ConnectionState::RESPONSE) {
      state_response(conn);
    }
  }

  void state_request(ConnectionSharedPtr conn) {
    LOG_DEBUG("state_request: " + std::to_string(conn->fd));
    do {
      ssize_t rc = 0;
      do {
        ssize_t cap = sizeof(conn->rbuf) - conn->rbuf_size;
        LOG_DEBUG("Reading data, capacity: " + std::to_string(cap));
        rc = read(conn->fd, &conn->rbuf[conn->rbuf_size], cap);
      } while (rc < 0 && errno == EINTR);

      if (rc < 0) {
        if (errno == EAGAIN) {
          // Got EAGAIN, stop it
          break;
        } else {
          LOG_DEBUG("read() error" + std::to_string(errno));
          break;
        }
      }

      if (rc == 0) {
        if (conn->rbuf_size > 0) {
          LOG_DEBUG("Unexpected EOF");
        } else {
          LOG_DEBUG("EOF");
        }
        conn->state = ConnectionState::END;
        break;
      }

      conn->rbuf_size += size_t(rc);
      while (try_one_request(conn)) {
      }
    } while (conn->state == ConnectionState::REQUEST);
  }

  bool try_one_request(ConnectionSharedPtr conn) {
    if (conn->rbuf_size < 4) {
      // Not enough data in the buffer, will retry in the next iterator
      return false;
    }
    u_int32_t len = 0;
    memcpy(&len, &conn->rbuf[0], 4);
    LOG_DEBUG("Request length: " + std::to_string(len));
    if (len > k_max_msg) {
      LOG_DEBUG("Too long");
      conn->state = ConnectionState::END;
      return false;
    }
    if (conn->rbuf_size < 4 + len) {
      // Not enough data in the buffer
      return false;
    }

    LOG_INFO(std::to_string(++totalConnected) + " Client says: " +
             std::string(reinterpret_cast<char *>(&conn->rbuf[4])));
    // Response to client
    memcpy(&conn->wbuf[0], &len, 4);
    memcpy(&conn->wbuf[4], &conn->rbuf[4], len);
    conn->wbuf_size = 4 + len;
    conn->state = ConnectionState::RESPONSE;

    // Remove the processed data from the read buffer
    size_t remaining = conn->rbuf_size - (4 + len);
    if (remaining > 0) {
      memmove(&conn->rbuf[0], &conn->rbuf[4 + len], remaining);
    }
    conn->rbuf_size = remaining;
    //

    state_response(conn);
    return conn->state == ConnectionState::REQUEST;
  }

  void state_response(ConnectionSharedPtr conn) {
    LOG_DEBUG("state_response: " + std::to_string(conn->fd));
    do {
      ssize_t rv = 0;
      do {
        ssize_t cap = conn->wbuf_size - conn->wbuf_sent;
        rv = write(conn->fd, &conn->wbuf[conn->wbuf_sent], cap);
      } while (rv < 0 && errno == EINTR);

      if (rv < 0) {
        if (errno == EAGAIN) {
          // Got EAGAIN, stop it
          break;
        } else {
          LOG_DEBUG("write() error: " + std::to_string(errno));
          conn->state = ConnectionState::END;
          break;
        }
      }
      conn->wbuf_sent += size_t(rv);
      if (conn->wbuf_sent == conn->wbuf_size) {
        conn->state = ConnectionState::END;
        conn->wbuf_sent = 0;
        break;
      }
      // Still got some data in wbuf, could try to continue writing
    } while (true);
  }

  void accept_new_conn() {
    socklen_t addrlen = sizeof(addr);
    auto connFD = accept(serverFD, (struct sockaddr *)&addr, &addrlen);
    if (connFD == -1) {
      LOG_INFO("accept failed: " + std::to_string(errno));
      exit(EXIT_FAILURE);
    }
    utils::set_fb_nonblocking(connFD);
    epoll_event ev;
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = connFD;
    if (epoll_ctl(epollFD, EPOLL_CTL_ADD, connFD, &ev) == -1) {
      LOG_INFO("epoll_ctl: conn_sock");
      exit(EXIT_FAILURE);
    }
    LOG_DEBUG("accept_new_conn with fd " + std::to_string(connFD));
    ConnectionSharedPtr newConn = std::make_shared<Connection>();
    newConn->fd = connFD;
    newConn->state = ConnectionState::REQUEST;

    fd2Conn[connFD] = newConn;
  }

public:
  int serverFD;
  int epollFD;
  FDConnectionMap fd2Conn;
  struct sockaddr_in addr;

  int totalConnected = 0;
};

// Server object
Server::Server() { _impl = std::make_shared<ServerImpl>(); }
Server::~Server() {}
bool Server::init(int port) { return _impl->init(port); }
bool Server::start() { return _impl->start(); }
bool Server::stop() { return _impl->stop(); }
bool Server::deinit() { return _impl->deinit(); }
//
//
