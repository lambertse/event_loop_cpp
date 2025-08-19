#include "server_impl.h"

int main() {
  ServerImpl server;
  if (server.init()) {
    server.start();
  } else {
    return -1;
  }
}
