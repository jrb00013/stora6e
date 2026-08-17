#include "stora6e/scan_manager.hpp"

#include <algorithm>

#include "stora6e/scanner.hpp"

namespace stora6e {

ScanManager::~ScanManager() {
  // A finished-but-unjoined worker thread is still joinable(), and a
  // joinable std::thread destructor calls std::terminate. startScan() only
  // joins the *previous* worker when a new scan begins, so a ScanManager
  // that is destroyed after a scan completed (its common lifetime, e.g. at
  // process shutdown) must join here instead.
  cancel_.store(true);
  if (worker_.joinable()) worker_.join();
}

ScanStatus ScanManager::status() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return status_;
}

ScanProgress ScanManager::progress() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return progress_;
}

std::vector<ScanEntry> ScanManager::results() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return results_;
}

std::optional<std::string> ScanManager::lastError() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return last_error_;
}

bool ScanManager::startScan(const ScanConfig& config) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (status_ == ScanStatus::Running) return false;

  if (worker_.joinable()) worker_.join();

  cancel_.store(false);
  results_.clear();
  last_error_.reset();
  status_ = ScanStatus::Running;
  progress_ = ScanProgress{};

  worker_ = std::thread([this, config]() { runWorker(config); });
  return true;
}

void ScanManager::cancelScan() {
  cancel_.store(true);
}

void ScanManager::runWorker(ScanConfig config) {
  std::vector<ScanEntry> local_results;
  ScanProgress local_progress;

  try {
    Scanner scanner(std::move(config));
    scanner.run(
        local_results, local_progress, cancel_,
        [this](const ScanProgress& p) {
          std::lock_guard<std::mutex> lock(mutex_);
          progress_ = p;
        });
  } catch (const std::exception& ex) {
    std::lock_guard<std::mutex> lock(mutex_);
    status_ = ScanStatus::Error;
    last_error_ = ex.what();
    if (worker_.joinable()) {
      // joined from startScan next time
    }
    return;
  }

  std::lock_guard<std::mutex> lock(mutex_);
  results_ = std::move(local_results);
  progress_ = local_progress;
  status_ = cancel_.load() ? ScanStatus::Cancelled
                           : (last_error_ ? ScanStatus::Error : ScanStatus::Complete);
}

ScanManager::Summary ScanManager::summary() const {
  std::lock_guard<std::mutex> lock(mutex_);
  Summary s;
  s.total_entries = results_.size();
  for (const auto& e : results_) {
    s.total_reclaimable_bytes += e.size_bytes;
    const int idx = static_cast<int>(e.category);
    if (idx >= 0 && idx < 8) s.by_category[idx] += e.size_bytes;
  }
  return s;
}

}  // namespace stora6e
