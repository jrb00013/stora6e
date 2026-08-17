#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <filesystem>
#include <thread>

#include "stora6e/scan_manager.hpp"
#include "stora6e/scanner.hpp"
#include "stora6e/util.hpp"

namespace fs = std::filesystem;

using namespace stora6e;
using namespace std::chrono_literals;

namespace {

void waitUntilNotRunning(const ScanManager& mgr, std::chrono::milliseconds timeout = 5s) {
  const auto deadline = std::chrono::steady_clock::now() + timeout;
  while (mgr.status() == ScanStatus::Running && std::chrono::steady_clock::now() < deadline) {
    std::this_thread::sleep_for(1ms);
  }
}

ScanConfig emptyRootConfig() {
  ScanConfig cfg;
  // A root that doesn't exist: the walk finishes almost instantly, which
  // keeps these tests fast without depending on real filesystem contents.
  cfg.roots = {"/nonexistent-stora6e-test-root"};
  cfg.max_results = 10;
  return cfg;
}

}  // namespace

TEST_CASE("a fresh ScanManager starts Idle with no results or error", "[scan_manager]") {
  ScanManager mgr;
  REQUIRE(mgr.status() == ScanStatus::Idle);
  REQUIRE(mgr.results().empty());
  REQUIRE_FALSE(mgr.lastError().has_value());
}

TEST_CASE("startScan transitions Idle -> Running -> Complete", "[scan_manager]") {
  ScanManager mgr;
  REQUIRE(mgr.startScan(emptyRootConfig()));

  waitUntilNotRunning(mgr);

  REQUIRE(mgr.status() == ScanStatus::Complete);
  REQUIRE_FALSE(mgr.lastError().has_value());
}

TEST_CASE("startScan refuses to start a second scan while one is running", "[scan_manager]") {
  ScanManager mgr;
  ScanConfig cfg;
  // Scan a root likely to contain enough entries that the first scan is
  // still running when we attempt the second startScan() immediately after.
  cfg.roots = {homeDirectory()};
  cfg.max_depth = 64;

  REQUIRE(mgr.startScan(cfg));
  const bool second = mgr.startScan(cfg);

  // Whether or not we win the race against the worker thread, startScan()
  // must never report success while status() says Running unless the first
  // call already finished — i.e. it's never true that both calls "started".
  if (mgr.status() == ScanStatus::Running) {
    REQUIRE_FALSE(second);
  }

  mgr.cancelScan();
  waitUntilNotRunning(mgr);
}

TEST_CASE("cancelScan moves a running scan to Cancelled", "[scan_manager]") {
  ScanManager mgr;
  ScanConfig cfg;
  cfg.roots = {homeDirectory()};
  cfg.max_depth = 64;

  REQUIRE(mgr.startScan(cfg));
  mgr.cancelScan();
  waitUntilNotRunning(mgr);

  const auto status = mgr.status();
  REQUIRE((status == ScanStatus::Cancelled || status == ScanStatus::Complete));
}

TEST_CASE("a new scan clears results and error state from a previous run", "[scan_manager]") {
  ScanManager mgr;
  REQUIRE(mgr.startScan(emptyRootConfig()));
  waitUntilNotRunning(mgr);
  REQUIRE(mgr.status() == ScanStatus::Complete);

  REQUIRE(mgr.startScan(emptyRootConfig()));
  REQUIRE_FALSE(mgr.lastError().has_value());
  waitUntilNotRunning(mgr);
  REQUIRE(mgr.status() == ScanStatus::Complete);
}

TEST_CASE("summary reflects zero entries after scanning a nonexistent root", "[scan_manager]") {
  ScanManager mgr;
  REQUIRE(mgr.startScan(emptyRootConfig()));
  waitUntilNotRunning(mgr);

  const auto summary = mgr.summary();
  REQUIRE(summary.total_entries == 0);
  REQUIRE(summary.total_reclaimable_bytes == 0);
}

TEST_CASE("an unexpected (non-benign) filesystem error surfaces as ScanStatus::Error",
          "[scan_manager][scanner]") {
  // A root whose name exceeds the filesystem's name-length limit is a
  // deterministic way to provoke a non-benign std::error_code
  // (ENAMETOOLONG) from fs::exists() without needing real permission
  // changes or a filesystem race — this is exactly the kind of failure
  // that used to be silently swallowed as an empty, "successful" scan.
  const std::string too_long_name(5000, 'a');
  const auto bad_root = (fs::temp_directory_path() / too_long_name).string();

  ScanConfig cfg;
  cfg.roots = {bad_root};
  cfg.max_results = 10;

  ScanManager mgr;
  REQUIRE(mgr.startScan(cfg));
  waitUntilNotRunning(mgr);

  REQUIRE(mgr.status() == ScanStatus::Error);
  REQUIRE(mgr.lastError().has_value());
  REQUIRE(mgr.lastError()->find("filesystem error") != std::string::npos);
}

TEST_CASE("Scanner::lastError is empty after a clean scan of a real, empty directory",
          "[scanner]") {
  const auto dir = fs::temp_directory_path() / "stora6e_test_clean_dir";
  std::error_code ec;
  fs::remove_all(dir, ec);
  fs::create_directories(dir, ec);

  ScanConfig cfg;
  cfg.roots = {dir.string()};
  cfg.max_results = 10;

  Scanner scanner(cfg);
  std::vector<ScanEntry> results;
  ScanProgress progress;
  std::atomic<bool> cancel{false};

  scanner.run(results, progress, cancel, nullptr);

  REQUIRE(scanner.lastError().empty());

  fs::remove_all(dir, ec);
}
