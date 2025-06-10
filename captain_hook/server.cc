#include "captain_hook/server.h"

#include <chrono>
#include <cstdlib>
// Remove later
#include <thread>

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
  const char* port = std::getenv(kServerPortEnvVar.c_str());
  if (TSB_LIKELY(port == NULL)) {
    shared = std::make_shared<IPCServer>(kDefaultServerPort);
  } else {
    shared = std::make_shared<IPCServer>(std::atoi(port));
  }
  return shared;
}

IPCServer::IPCServer(int port) : port_(port) {}

void IPCServer::RunForever() {
  while (!exit_) {
    std::this_thread::sleep_for(std::chrono::seconds(5));
  }
}

}  // namespace captain_hook
