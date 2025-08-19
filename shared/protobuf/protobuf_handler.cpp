#include "proto/request.pb.h"
#include "protobuf_handler.h"

request::Request ProtobufHandler::deserialize(char* data, int len) {
  request::Request req;
  if (!req.ParseFromArray(data, len)) {
    req.set_msg("Failed to parse request");
  }
  return req;
}
std::vector<char> ProtobufHandler::serialize(request::Request object) {
  std::vector<char> buffer(object.ByteSizeLong());
  if (!object.SerializeToArray(buffer.data(), buffer.size())) {
    return {};
  }
  return buffer;
}
