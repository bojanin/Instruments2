#include "state.h"

#include <csignal>
#include <format>

namespace bandicoot {
Program::Program() : path_() {};

Program::~Program() { Stop(); }

void Program::Start() {
  std::string_view s = path_;
  std::printf(
      "%s\n",
      std::format("Starting program {} sizeof path: {}", s, s.size()).c_str());
  // Do Something
}

void Program::Stop() {
  if (pid_ != -1) {
    kill(pid_, 9);
  }
}

void AddReport(int report) {}

}  // namespace bandicoot
