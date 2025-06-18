#include <grpc++/grpc++.h>
#include <pbtypes/instruments2.grpc.pb.h>
#include <tsb/ipc.h>

#include <cstdlib>

namespace tsb {

// Shared will construct the server automatically if it hasn't been create
// already.
void IPCServer::Create() { (void)IPCServer::Shared(); }
std::shared_ptr<IPCServer> IPCServer::Shared() {
  static std::shared_ptr<IPCServer> shared;

  if (TSB_LIKELY(shared)) {
    return shared;
  }
  const char* port = std::getenv(tsb::kServerPortEnvVar);
  if (TSB_LIKELY(port == NULL)) {
    shared = std::make_shared<IPCServer>(tsb::kDefaultServerPort);
  } else {
    shared = std::make_shared<IPCServer>(std::atoi(port));
  }
  return shared;
}

IPCServer::IPCServer(int port) : port_(port) {
  SPDLOG_INFO("using protobuf {}", google::protobuf::internal::VersionString(
                                       GOOGLE_PROTOBUF_VERSION));
  SPDLOG_INFO("TsanReport: {}",
              instruments2::TsanReport::descriptor()->DebugString());
}
IPCServer::~IPCServer() {
  SPDLOG_INFO("IPC Server destructing");
  grpc_server_->Shutdown();
}

::grpc::Status IPCServer::OnSanitizerReport(grpc::ServerContext* context,
                                            const instruments2::TsanReport* msg,
                                            instruments2::Void* out) {
  SPDLOG_INFO("GOt Something");
  SPDLOG_INFO("RECEIVED: {}", msg->DebugString());
  return grpc::Status::OK;
}
void IPCServer::SetExitFlag() { grpc_server_->Shutdown(); }

void IPCServer::RunForever() {
  const std::string server_address = fmt::format("127.0.0.1:{}", port_);
  grpc::EnableDefaultHealthCheckService(true);

  grpc::ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(this);
  grpc_server_ = builder.BuildAndStart();
  SPDLOG_INFO("Server Listening on: {}", server_address);
  grpc_server_->Wait();
  SPDLOG_INFO("Server Shutting down...");
}

}  // namespace tsb
