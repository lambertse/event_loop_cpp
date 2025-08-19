#include "event_loop/shared/protobuf_handler.h"

request::Request ProtobufHandler::deserialize(const std::string& data) {
  request::Request req;
  if (!req.ParseFromArray(data.data(), data.size())) {
    req.set_msg("Failed to parse request");
  }
  return req;
}
std::string ProtobufHandler::serialize(const request::Request& object) {
  std::string buffer;
  buffer.resize(object.ByteSizeLong());
  if (!object.SerializeToArray(buffer.data(), buffer.size())) {
    return {};
  }
  return buffer;
}
