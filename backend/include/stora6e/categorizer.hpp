#pragma once

#include "stora6e/types.hpp"

namespace stora6e {

class Categorizer {
 public:
  explicit Categorizer(const ScanConfig& config);

  void classifyFile(const std::string& path, std::uint64_t size, std::int64_t mtime,
                    std::vector<ScanEntry>& out);
  void finalizeDuplicates(std::vector<ScanEntry>& entries);

 private:
  ScanConfig config_;
  std::int64_t old_cutoff_unix_ = 0;
};

}  // namespace stora6e
