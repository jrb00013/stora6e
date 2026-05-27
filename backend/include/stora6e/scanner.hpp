#pragma once

#include <atomic>
#include <functional>

#include "stora6e/categorizer.hpp"
#include "stora6e/types.hpp"

namespace stora6e {

class Scanner {
 public:
  explicit Scanner(ScanConfig config);

  void run(std::vector<ScanEntry>& results, ScanProgress& progress,
           const std::atomic<bool>& cancel_flag,
           std::function<void(const ScanProgress&)> on_progress);

 private:
  ScanConfig config_;
  Categorizer categorizer_;

  void walk(const std::string& root, int depth, std::vector<ScanEntry>& results,
            ScanProgress& progress, const std::atomic<bool>& cancel_flag,
            std::function<void(const ScanProgress&)> on_progress);
};

}  // namespace stora6e
