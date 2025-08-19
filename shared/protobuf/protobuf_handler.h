#pragma once

#include <vector>

#include "proto/request.pb.h"
class ProtobufHandler {
 public:
  static request::Request deserialize(char* data, int len);
  static std::vector<char> serialize(request::Request object);
  size_t get_max_message_size() const;
};
