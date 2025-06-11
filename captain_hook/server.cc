#include "captain_hook/server.h"

#include <fmt/format.h>
#include <grpc++/grpc++.h>
#include <pbtypes/bandicoot.grpc.pb.h>
#include <spdlog/spdlog.h>

#include <cstdlib>

#include "captain_hook/constants.h"
namespace captain_hook {

// Shared will construct the server automatically if it hasn't been create
// already.
void IPCServer::Create() { (void)IPCServer::Shared(); }
std::shared_ptr<IPCServer> IPCServer::Shared() {
  static std::shared_ptr<IPCServer> shared;

  if (TSB_LIKELY(shared)) {
    return shared;
  }
  const char* port = std::getenv(kServerPortEnvVar);
  if (TSB_LIKELY(port == NULL)) {
    shared = std::make_shared<IPCServer>(kDefaultServerPort);
  } else {
    shared = std::make_shared<IPCServer>(std::atoi(port));
  }
  return shared;
}

IPCServer::IPCServer(int port) : port_(port) {}

void IPCServer::SetExitFlag() { grpc_server_->Shutdown(); }

void IPCServer::RunForever() {
  std::string server_address(fmt::format("127.0.0.1:{}", port_));

  grpc::ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(this);
  grpc_server_ = builder.BuildAndStart();
  SPDLOG_INFO("Server Listening on: {}", server_address);
  grpc_server_->Wait();
}

}  // namespace captain_hook
