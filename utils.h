#pragma once
#include <cerrno>
#include <fcntl.h>
#include <iostream>

namespace utils {
static void set_fb_nonblocking(int &fd) {
  errno = 0;
  int flags = fcntl(fd, F_GETFL, 0);
  if (errno) {
    std::cout << "fcntl get error\n";
    return;
  }

  fcntl(fd, F_SETFL, flags | O_NONBLOCK);
  if (errno) {
    std::cout << "fcntl set error\n";
    return;
  }
}
} // namespace utils
