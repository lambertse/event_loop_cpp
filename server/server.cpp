#include "server.h"
#include "server_impl.h"

Server::Server() { _impl = std::make_shared<ServerImpl>(); }
Server::~Server() {}
bool Server::init(int port) { return _impl->init(port); }
bool Server::start() { return _impl->start(); }
bool Server::stop() { return _impl->stop(); }
bool Server::deinit() { return _impl->deinit(); }
