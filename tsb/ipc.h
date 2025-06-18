#pragma once
#include <fmt/format.h>
#include <grpc++/grpc++.h>
#include <pbtypes/instruments2.grpc.pb.h>
#include <spdlog/spdlog.h>
#include <tsb/constants.h>
#include <tsb/macros.h>

#include <memory>

namespace tsb {

class IPCClient {
 public:
  IPCClient() {
    const std::string server_ip =
        fmt::format("127.0.0.1:{}", tsb::kDefaultServerPort);
    std::shared_ptr<grpc::Channel> channel =
        grpc::CreateChannel(server_ip, grpc::InsecureChannelCredentials());

    stub_ = instruments2::DesktopApp::NewStub(channel);
    SPDLOG_INFO("Initialized client that talks to {}", server_ip);
  }

  std::unique_ptr<instruments2::DesktopApp::Stub> stub_;
};

// IPC server that talks to instruments2 via grpc.
// Server will listen on instruments2_SERVER_PORT thats in the users env
class IPCServer : public instruments2::DesktopApp::Service {
 public:
  // Will create a server if it hasnt been created already
  void SetExitFlag();
  void RunForever();
  int32_t Port() const { return port_; }
  ::grpc::Status OnSanitizerReport(grpc::ServerContext* context,
                                   const instruments2::TsanReport* msg,
                                   instruments2::Void* out) override;

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

}  // namespace tsb
