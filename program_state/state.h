#pragma once
#include <string>
#include <vector>

namespace bandicoot {
class Program {
 public:
  Program();
  ~Program();  // kill child?

  Program operator=(const Program& other) = delete;
  Program operator=(const Program&& other) = delete;

  void Start();
  void Stop();
  void AddReport(int report);

  char* ImGuiRawBuf() { return path_; }
  ssize_t BufSize() const { return sizeof(path_); }

 private:
  int pid_ = -1;
  char path_[512] = "/path/to/progran/executable";
  std::vector<int> sanitize_reports_;
};
}  // namespace bandicoot
