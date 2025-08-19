#include <unistd.h>

#include "connection_manager.h"
#include "define.h"
#include "proto/request.pb.h"
#include "shared/logging.h"

ConnectionManager::ConnectionManager() {}

void ConnectionManager::remove_connection(int fd) {}

void ConnectionManager::handle_connection_io(ConnectionSharedPtr conn) {
  if (conn->state == ConnectionState::REQUEST) {
    state_request(conn);
  } else if (conn->state == ConnectionState::RESPONSE) {
    state_response(conn);
  }
}
void ConnectionManager::set_message_handler(MessageHandler handler) {
  _message_handler = std::move(handler);
}

void ConnectionManager::state_request(ConnectionSharedPtr conn) {
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

void ConnectionManager::state_response(ConnectionSharedPtr conn) {
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

bool ConnectionManager::try_one_request(ConnectionSharedPtr conn) {
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
  request::Request req = _protocol_handler->deserialize(&conn->rbuf[4], len);
  static int count = 0;
  LOG_INFO(std::to_string(count++) + ". Incomming message: " + req.msg());

  request::Response res;
  res.set_msg("Responnse for message: " + req.msg() + " from server");
  char serializedData[k_max_msg];
  size_t resLen = res.ByteSizeLong();
  if (resLen > k_max_msg - 4) {
    LOG_INFO("Response too large");
    conn->state = ConnectionState::END;
    return false;
  }
  if (!res.SerializeToArray(serializedData, resLen)) {
    LOG_INFO("Failed to serialize response");
    conn->state = ConnectionState::END;
    return false;
  }

  conn->wbuf_size = resLen + 4;  // +4 for the length prefix
  memcpy(&conn->wbuf[0], &resLen, 4);
  memcpy(&conn->wbuf[4], serializedData, resLen);

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
