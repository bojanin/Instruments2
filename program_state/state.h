#pragma once
#include <string>
#include <vector>

namespace bandicoot {
class Program {
 public:
  explicit Program(const std::string path);
  ~Program();  // kill child?

  Program operator=(const Program& other) = delete;
  Program operator=(const Program&& other) = delete;

  void Start();
  void AddReport(int report);

 private:
  const std::string path_;
  std::vector<int> sanitize_reports_;
};
}  // namespace bandicoot
