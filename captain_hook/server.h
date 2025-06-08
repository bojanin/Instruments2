#pragma once
#include <tsb/macros.h>

#include <atomic>
#include <memory>

namespace captain_hook {

// IPC server that talks to Bandicoot via grpc.
// Server will listen on BANDICOOT_SERVER_PORT thats in the users env
class IPCServer {
 public:
  // Will create a server if it hasnt been created already
  void SetExitFlag(bool exit) { exit_ = exit; }
  void RunForever();
  int32_t Port() const { return port_; }

  // Init related functions
  IPCServer(int port);
  static std::shared_ptr<IPCServer> Shared();
  static void CreateServer();
  TSB_DISALLOW_COPY_AND_ASSIGN(IPCServer);
  TSB_DISALLOW_MOVE(IPCServer);

 private:
  const int port_;
  std::atomic_bool exit_ = false;
};

}  // namespace captain_hook
