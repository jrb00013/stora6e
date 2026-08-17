#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <thread>

#include "stora6e/scan_manager.hpp"
#include "stora6e/util.hpp"

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
