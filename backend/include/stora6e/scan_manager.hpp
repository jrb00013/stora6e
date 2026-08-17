#pragma once

#include <atomic>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

#include "stora6e/types.hpp"

namespace stora6e {

class ScanManager {
 public:
  ~ScanManager();

  ScanStatus status() const;
  ScanProgress progress() const;
  std::vector<ScanEntry> results() const;
  std::optional<std::string> lastError() const;

  bool startScan(const ScanConfig& config);
  void cancelScan();

  struct Summary {
    std::uint64_t total_entries = 0;
    std::uint64_t total_reclaimable_bytes = 0;
    std::uint64_t by_category[8]{};
  };
  Summary summary() const;

 private:
  mutable std::mutex mutex_;
  ScanStatus status_ = ScanStatus::Idle;
  ScanProgress progress_;
  std::vector<ScanEntry> results_;
  std::optional<std::string> last_error_;
  std::thread worker_;
  std::atomic<bool> cancel_{false};

  void runWorker(ScanConfig config);
};

}  // namespace stora6e
