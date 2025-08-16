
#include <netinet/in.h>
#include <shared/logging.h>
#include <shared/utils.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <string>
#include <thread>
#include <vector>

static int send_req(const int &fd, const char *text) {
  LOG_DEBUG("Sending request: " + std::string(text));
  int len = strlen(text);
  write(fd, &len, 4);
  while (len > 0) {
    auto rv = write(fd, text, len);
    if (rv <= 0) return -1;

    len -= rv;
    text += rv;
  }
  return 0;
}

static int read_res(const int &fd, char *text) {
  int len = 0;
  auto rv = read(fd, &len, 4);
  LOG_DEBUG("Reading response with len " + std::to_string(len));
  while (len > 0) {
    rv = read(fd, text, len);
    if (rv <= 0) return -1;
    text += rv;
    len -= rv;
  }
  text[len] = '\0';
  return 0;
}

int handle_a_client() {
  LOG_DEBUG("Client started");
  const char *msg = "hello!!!";
  int fd = socket(AF_INET, SOCK_STREAM, 0);

  if (fd < 0) {
    LOG_DEBUG("can not create fd");
    return -1;
  }

  const int port = 8100;
  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = htonl(0);

  LOG_INFO("Connecting to server with fd: " + std::to_string(fd));
  int rv = connect(fd, (const struct sockaddr *)&addr, sizeof(addr));
  LOG_DEBUG("connect returned: " + std::to_string(rv));

  if (rv < 0) {
    LOG_DEBUG("connect failed: " + std::to_string(errno));
    close(fd);
    return -1;
  }

  int err = send_req(fd, msg);
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
  LOG_INFO("Res: " + std::string(reinterpret_cast<char *>(&buf[0])));

L_DONE:
  LOG_DEBUG("Closing fd: " + std::to_string(fd));
  close(fd);
  return 0;
}

int main() {
  logging::log_level = logging::DEBUG;
  std::vector<std::thread> threads;
  const int numClients = 20;
  for (int i = 0; i < numClients; i++) {
    threads.emplace_back(handle_a_client);
  }
  for (auto &t : threads) {
    if (t.joinable()) {
      t.join();
    }
  }
}
