
#include <netinet/in.h>
#include <shared/logging.h>
#include <shared/utils.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <string>
#include <thread>
#include <vector>

#include "proto/request.pb.h"
#include "shared/protobuf/protobuf_handler.h"

static int send_req(const int &fd, const std::string &data) {
  LOG_INFO("Sending serialized request of size: " +
           std::to_string(data.size()));
  int len = data.length();
  write(fd, &len, 4);
  const char *buf = data.data();
  while (len > 0) {
    auto rv = write(fd, buf, len);
    if (rv <= 0) return -1;
    len -= rv;
    buf += rv;
  }
  return 0;
}

static int read_res(const int &fd, char *text) {
  int len = 0;
  auto rv = read(fd, &len, 4);
  LOG_INFO("Reading response with len " + std::to_string(len));
  while (len > 0) {
    rv = read(fd, text, len);
    if (rv <= 0) return -1;
    text += rv;
    len -= rv;
  }
  text[len] = '\0';
  return 0;
}

request::Response *handle_a_client(const request::Request &req) {
  LOG_DEBUG("Client started");

  int fd = socket(AF_INET, SOCK_STREAM, 0);
  request::Response *res = nullptr;
  if (fd < 0) {
    LOG_DEBUG("can not create fd");
    return nullptr;
  }

  const int port = 8100;
  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = htonl(0);

  LOG_DEBUG("Connecting to server with fd: " + std::to_string(fd));
  int rv = connect(fd, (const struct sockaddr *)&addr, sizeof(addr));
  LOG_DEBUG("connect returned: " + std::to_string(rv));

  if (rv < 0) {
    LOG_DEBUG("connect failed: " + std::to_string(errno));
    close(fd);
    return nullptr;
  }
  auto sendbuf = ProtobufHandler::serialize(req);
  int err = send_req(fd, sendbuf);
  if (err) {
    LOG_DEBUG("send_req failed: " + std::to_string(errno));
    goto L_DONE;
  }
  char buf[260];
  err = read_res(fd, buf);
  if (err) {
    LOG_DEBUG("read_res failed: " + std::to_string(errno));
    goto L_DONE;
  }
  res = new request::Response();
  res->ParseFromArray(buf, strlen(buf));

L_DONE:
  LOG_DEBUG("Closing fd: " + std::to_string(fd));
  close(fd);
  return res;
}

int main() {
  std::vector<std::thread> threads;
  // Random vector string of 20 random string:
  std::vector<std::string> random_strings = {"Hello, World!",
                                             "Protobuf is great!",
                                             "Multithreading in C++",
                                             "Network programming",
                                             "Socket communication",
                                             "Concurrency",
                                             "Thread safety",
                                             "C++ best practices",
                                             "Error handling",
                                             "Performance optimization",
                                             "Data serialization",
                                             "Protocol buffers",
                                             "Asynchronous programming",
                                             "Event-driven architecture",
                                             "Client-server model",
                                             "Network protocols",
                                             "TCP/IP stack",
                                             "Socket programming in C++",
                                             "C++ networking libraries",
                                             "Cross-platform development"};
  const int numClients = 20;
  for (int i = 0; i < numClients; i++) {
    threads
        .emplace_back([&]() {
          request::Request req;
          req.set_msg(random_strings[i]);
          auto res = handle_a_client(req);
          std::cout << "Response from client: "
                    << (res ? res->msg() : "No response") << std::endl;
        })
        .join();
  }
  for (auto &t : threads) {
    if (t.joinable()) {
      t.join();
    }
  }
}
