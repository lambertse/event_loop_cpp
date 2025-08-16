#include "server.h"
#include "shared/logging.h"

int main() {
  Server server;
  if (server.init(8100)) {
    server.start();
  } else {
    return -1;
  }
}
