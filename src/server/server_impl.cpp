#include <unistd.h>

#include <memory>

#include "event_loop/config.h"
#include "event_loop/connection_manager.h"
#include "event_loop/poll.h"
#include "event_loop/server_impl.h"
#include "event_loop/shared/logging.h"

ServerImpl::ServerImpl(ServerConfig config) : _server_fd(-1), _config(config) {
  _poll = std::make_unique<Poll>();
  _connection_manager = std::make_unique<ConnectionManager>();
  _poll->set_connection_io_callback(
      std::bind(&ServerImpl::add_conection, this, std::placeholders::_1));
}

ServerImpl::~ServerImpl() {}

bool ServerImpl::init() {
  if (!setup_socket()) {
    LOG_INFO("Failed to setup socket");
    return false;
  }
  LOG_INFO("Server is listening on port " + std::to_string(_config.port));
  _poll->set_connection_io_callback(
      std::bind(&ServerImpl::add_conection, this, std::placeholders::_1));
  return _poll->init(_server_fd);
}

bool ServerImpl::start() {
  if (_poll && !_poll->start()) {
    LOG_INFO("Failed to start the poll");
    return false;
  }
  return true;
}

bool ServerImpl::stop() {
  if (_poll) {
    _poll->stop();
  }
  for (auto &[fd, conn] : _fd_2_conn) {
    if (conn) {
      close(fd);
    }
  }
  _fd_2_conn.clear();
  if (_server_fd >= 0) {
    close(_server_fd);
    _server_fd = -1;
  }
  return true;
}

bool ServerImpl::deinit() { return true; }

// Private section
void ServerImpl::add_conection(int fd) {
  if (fd == _server_fd) {
    accept_new_connection();
  } else {
    handle_connection_io(fd);
  }
}

void ServerImpl::accept_new_connection() {
  socklen_t addrlen = sizeof(_addr);
  auto connFD = accept(_server_fd, (struct sockaddr *)&_addr, &addrlen);
  if (connFD == -1) {
    LOG_INFO("accept failed: " + std::to_string(errno));
    exit(EXIT_FAILURE);
  }
  utils::set_fb_nonblocking(connFD);
  _poll->add_connection(connFD);

  LOG_DEBUG("accept_new_conn with fd " + std::to_string(connFD));
  ConnectionSharedPtr newConn = std::make_shared<Connection>();
  newConn->fd = connFD;
  newConn->state = ConnectionState::REQUEST;

  _fd_2_conn[connFD] = newConn;
}

void ServerImpl::handle_connection_io(int fd) {
  if (_fd_2_conn.count(fd) <= 0) {
    LOG_INFO("Connection not found for fd: " + std::to_string(fd));
    return;
  }
  ConnectionSharedPtr conn = _fd_2_conn[fd];

  if (conn->state == ConnectionState::END) {
    LOG_DEBUG("Connection ended for fd: " + std::to_string(fd));
    close(fd);
    _fd_2_conn.erase(fd);
    return;
  }
  _connection_manager->handle_connection_io(conn);
}

bool ServerImpl::setup_socket() {
  _server_fd = socket(AF_INET, SOCK_STREAM, 0);
  int val = false;
  setsockopt(_server_fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));
  if (_server_fd < 0) {
    LOG_DEBUG("fd is not valid " + std::to_string(_server_fd));
    return false;
  }
  utils::set_fb_nonblocking(_server_fd);

  _addr.sin_family = AF_INET;
  _addr.sin_port = htons(_config.port);
  _addr.sin_addr.s_addr = htonl(0);

  if (bind(_server_fd, (struct sockaddr *)&_addr, sizeof(_addr)) < 0) {
    LOG_DEBUG("bind failed");
    return false;
  }
  if (listen(_server_fd, 100) < 0) {
    LOG_DEBUG("listen failed");
    return false;
  }
  return true;
}
