#include "server.h"

int main() {
  Server server;
  if (server.init(8100)) {
    server.start();
  } else {
    return -1;
  }
}
