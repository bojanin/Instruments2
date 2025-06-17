#pragma once
#include <fmt/format.h>
#include <grpc++/grpc++.h>
#include <pbtypes/bandicoot.grpc.pb.h>
#include <spdlog/spdlog.h>
#include <tsb/macros.h>

#include <memory>

#include "captain_hook/constants.h"

namespace captain_hook {

class IPCClient {
 public:
  IPCClient() {
    const std::string server_ip =
        fmt::format("127.0.0.1:{}", kDefaultServerPort);
    std::shared_ptr<grpc::Channel> channel =
        grpc::CreateChannel(server_ip, grpc::InsecureChannelCredentials());

    stub_ = bandicoot::DesktopApp::NewStub(channel);
    SPDLOG_INFO("Initialized client that talks to {}", server_ip);
  }

  std::unique_ptr<bandicoot::DesktopApp::Stub> stub_;
};

// IPC server that talks to Bandicoot via grpc.
// Server will listen on BANDICOOT_SERVER_PORT thats in the users env
class IPCServer : public bandicoot::DesktopApp::Service {
 public:
  // Will create a server if it hasnt been created already
  void SetExitFlag();
  void RunForever();
  int32_t Port() const { return port_; }
  ::grpc::Status OnSanitizerReport(grpc::ServerContext* context,
                                   const bandicoot::TestMsg* msg,
                                   bandicoot::Void* out) override;

  // Init related functions
  IPCServer(int port);
  ~IPCServer();
  static std::shared_ptr<IPCServer> Shared();
  static void Create();
  TSB_DISALLOW_COPY_AND_ASSIGN(IPCServer);
  TSB_DISALLOW_MOVE(IPCServer);

 private:
  std::unique_ptr<grpc::Server> grpc_server_;
  const int port_;
};

}  // namespace captain_hook
