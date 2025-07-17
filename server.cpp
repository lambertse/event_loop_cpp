#include "server.h"
#include "server_define.h"
#include "utils.h"
#include <csignal>

#include <cstddef>
#include <cstring>
#include <map>
#include <memory>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <poll.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <vector>

class ServerImpl {
public:
  ServerImpl();
  ~ServerImpl();
  bool init() {
    fd = socket(AF_INET, SOCK_STREAM, 0);
    int val = false;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));
    if (fd < 0) {
      std::cout << "fd is not valid " << fd << std::endl;
      return false;
    }
    utils::set_fb_nonblocking(fd);

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    // Localhost address
    addr.sin_addr.s_addr = htonl(0);
    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
      std::cout << "bind failed" << std::endl;
      return false;
    }
    if (listen(fd, 5) < 0) {
      std::cout << "listen failed" << std::endl;
      return false;
    }
    std::cout << "Server is listening on port 8080" << std::endl;

    return true;
  }

  bool start() {
    std::vector<struct pollfd> poll_args;
    while (true) {
      poll_args.clear();
      struct pollfd lisFD = {fd, POLL_IN, 0};
      poll_args.push_back(lisFD);

      for (auto &[fd, con] : fd2Conn) {
        if (!con)
          continue;

        struct pollfd pfd = {};
        pfd.fd = con->fd;
        pfd.events =
            (con->state == ConnectionState::REQUEST) ? POLL_IN : POLL_OUT;
        pfd.events |= POLL_ERR;
        poll_args.push_back(pfd);
      }

      int rc = poll(poll_args.data(), nfds_t(poll_args.size()), 1000);
      if (rc < 0) {
        std::cout << "poll error: " << errno << std::endl;
        continue;
      }

      for (auto &p : poll_args) {
        if (!p.revents)
          continue;
        ConnectionSharedPtr con = fd2Conn[p.fd];
        connection_io(con);
        if (con->state == ConnectionState::END) {
          fd2Conn[p.fd] = nullptr;
          (void)close(p.fd);
        }
      }

      if (poll_args.front().revents) {
        accept_new_conn();
      }
    }
  }

  bool stop();

  bool deinit();

  void connection_io(ConnectionSharedPtr conn) {
    if (conn->state == ConnectionState::REQUEST) {
      state_request(conn);
    } else if (conn->state == ConnectionState::RESPONSE) {
      state_response(conn);
    }
  }

  void state_request(ConnectionSharedPtr conn) {
    do {
      ssize_t rc = 0;
      do {
        ssize_t cap = sizeof(conn->rbuf) - conn->rbuf_size;
        rc = read(conn->fd, &conn->rbuf[conn->rbuf_size], cap);
      } while (rc < 0 && errno == EINTR);

      if (rc < 0) {
        if (errno == EAGAIN) {
          // Got EAGAIN, stop it
          break;
        } else {
          std::cout << "read() error" << errno << std::endl;
          break;
        }
      }

      if (rc == 0) {
        if (conn->rbuf_size > 0) {
          std::cout << "Unexpected EOF\n";
        } else {
          std::cout << "EOF\n";
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
    if (len > k_max_msg) {
      std::cout << "Too long\n";
      conn->state = ConnectionState::END;
      return false;
    }
    if (conn->rbuf_size < 4 + len) {
      // Not enough data in the buffer
      return false;
    }

    std::cout << "Client says: " << conn->rbuf << std::endl;
    // Response to client
    memcpy(&conn->wbuf[0], &len, 4);
    memcpy(&conn->wbuf[0], &conn->rbuf[4], len);
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
          std::cout << "write() error: " << errno << std::endl;
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
    sockaddr_in client_addr{};
    socklen_t socklen = sizeof(client_addr);
    int connFD = accept(fd, (sockaddr *)&client_addr, &socklen);
    utils::set_fb_nonblocking(connFD);

    ConnectionSharedPtr newConn = std::make_shared<Connection>();
    newConn->fd = connFD;
    newConn->state = ConnectionState::REQUEST;

    fd2Conn[connFD] = newConn;
  }

public:
  int fd;
  FDConnectionMap fd2Conn;
};

// Server object
Server::Server() { _impl = std::make_shared<ServerImpl>(); }
Server::~Server() {}
bool Server::init() { return _impl->init(); }
bool Server::start() { return _impl->start(); }
bool Server::stop() { return _impl->stop(); }
bool Server::deinit() { return _impl->deinit(); }
//
//
