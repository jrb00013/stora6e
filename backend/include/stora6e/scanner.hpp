#pragma once

#include <atomic>
#include <functional>
#include <string>
#include <system_error>

#include "stora6e/categorizer.hpp"
#include "stora6e/types.hpp"

namespace stora6e {

class Scanner {
 public:
  explicit Scanner(ScanConfig config);

  void run(std::vector<ScanEntry>& results, ScanProgress& progress,
           const std::atomic<bool>& cancel_flag,
           std::function<void(const ScanProgress&)> on_progress);

  // Non-empty after run() if the filesystem walk hit unexpected errors
  // (i.e. std::error_code failures that are not simply "permission denied"
  // or "no such file" — both of which are expected/benign during a
  // best-effort walk of an unprivileged, possibly-changing filesystem).
  const std::string& lastError() const { return last_error_; }

 private:
  ScanConfig config_;
  Categorizer categorizer_;
  std::string last_error_;
  int error_count_ = 0;

  void walk(const std::string& root, int depth, std::vector<ScanEntry>& results,
            ScanProgress& progress, const std::atomic<bool>& cancel_flag,
            std::function<void(const ScanProgress&)> on_progress);

  void recordError(const std::string& path, const std::error_code& ec);
};

}  // namespace stora6e
