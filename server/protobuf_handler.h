#pragma once

#include <vector>

#include "proto/request.pb.h"
class ProtobufHandler {
 public:
  request::Request deserialize(uint8_t* data, int len);
  std::vector<uint8_t> serialize(request::Request object);
  size_t get_max_message_size() const;
};
