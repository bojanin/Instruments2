#pragma once
#include <pbtypes/bandicoot.grpc.pb.h>
#include <tsb/macros.h>

#include <memory>

namespace captain_hook {

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
                                   bandicoot::Void* out) override {
    abort();
    return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
  }

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
