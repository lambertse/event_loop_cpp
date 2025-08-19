#include "proto/request.pb.h"
#include "protobuf_handler.h"

request::Request ProtobufHandler::deserialize(uint8_t* data, int len) {
  request::Request req;
  if (!req.ParseFromArray(data, len)) {
    req.set_msg("Failed to parse request");
  }
  return req;
}
std::vector<uint8_t> ProtobufHandler::serialize(request::Request object) {
  std::vector<uint8_t> buffer(object.ByteSizeLong());
  if (!object.SerializeToArray(buffer.data(), buffer.size())) {
    return {};
  }
  return buffer;
}
