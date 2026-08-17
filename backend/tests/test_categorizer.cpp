#include <catch2/catch_test_macros.hpp>

#include "stora6e/categorizer.hpp"
#include "stora6e/util.hpp"

using namespace stora6e;

namespace {

ScanConfig baseConfig() {
  ScanConfig cfg;
  cfg.min_age_days = 90;
  cfg.min_size_mb = 50;
  cfg.scan_old = true;
  cfg.scan_large = true;
  cfg.scan_cache = true;
  cfg.scan_temp = true;
  cfg.scan_logs = true;
  cfg.scan_duplicates = false;
  cfg.scan_empty_dirs = false;
  return cfg;
}

bool hasCategory(const std::vector<ScanEntry>& entries, Category c) {
  for (const auto& e : entries) {
    if (e.category == c) return true;
  }
  return false;
}

}  // namespace

TEST_CASE("cache paths are classified as Cache", "[categorizer]") {
  Categorizer cat(baseConfig());
  std::vector<ScanEntry> out;
  cat.classifyFile("/home/user/.cache/thumbnails/foo.png", 1024, nowUnix(), out);
  REQUIRE(hasCategory(out, Category::Cache));
}

TEST_CASE("tmp paths and .tmp suffix are classified as Temp", "[categorizer]") {
  Categorizer cat(baseConfig());
  std::vector<ScanEntry> out;
  cat.classifyFile("/tmp/build/output.tmp", 10, nowUnix(), out);
  REQUIRE(hasCategory(out, Category::Temp));

  out.clear();
  cat.classifyFile("/home/user/scratch.temp", 10, nowUnix(), out);
  REQUIRE(hasCategory(out, Category::Temp));
}

TEST_CASE(".log files and /logs/ directories are classified as Log", "[categorizer]") {
  Categorizer cat(baseConfig());
  std::vector<ScanEntry> out;
  cat.classifyFile("/var/app/logs/service.log", 100, nowUnix(), out);
  REQUIRE(hasCategory(out, Category::Log));

  out.clear();
  cat.classifyFile("/home/user/app/access.log.1", 100, nowUnix(), out);
  REQUIRE(hasCategory(out, Category::Log));
}

TEST_CASE("files at or above min_size_mb are classified as Large", "[categorizer]") {
  auto cfg = baseConfig();
  cfg.min_size_mb = 50;
  Categorizer cat(cfg);
  std::vector<ScanEntry> out;

  const std::uint64_t just_under = 49ULL * 1024 * 1024;
  cat.classifyFile("/home/user/video.mp4", just_under, nowUnix(), out);
  REQUIRE_FALSE(hasCategory(out, Category::Large));

  out.clear();
  const std::uint64_t exactly_at = 50ULL * 1024 * 1024;
  cat.classifyFile("/home/user/video.mp4", exactly_at, nowUnix(), out);
  REQUIRE(hasCategory(out, Category::Large));
}

TEST_CASE("files older than min_age_days are classified as Old", "[categorizer]") {
  auto cfg = baseConfig();
  cfg.min_age_days = 90;
  Categorizer cat(cfg);
  std::vector<ScanEntry> out;

  const auto old_mtime = nowUnix() - (91LL * 86400);
  cat.classifyFile("/home/user/archive.zip", 100, old_mtime, out);
  REQUIRE(hasCategory(out, Category::Old));

  out.clear();
  const auto recent_mtime = nowUnix() - (10LL * 86400);
  cat.classifyFile("/home/user/recent.zip", 100, recent_mtime, out);
  REQUIRE_FALSE(hasCategory(out, Category::Old));
}

TEST_CASE("a file can match multiple categories at once", "[categorizer]") {
  auto cfg = baseConfig();
  cfg.min_size_mb = 1;
  Categorizer cat(cfg);
  std::vector<ScanEntry> out;

  const auto old_mtime = nowUnix() - (100LL * 86400);
  cat.classifyFile("/home/user/.cache/big.log", 5ULL * 1024 * 1024, old_mtime, out);

  REQUIRE(hasCategory(out, Category::Cache));
  REQUIRE(hasCategory(out, Category::Log));
  REQUIRE(hasCategory(out, Category::Large));
  REQUIRE(hasCategory(out, Category::Old));
}

TEST_CASE("disabling a scan flag suppresses that category", "[categorizer]") {
  auto cfg = baseConfig();
  cfg.scan_cache = false;
  Categorizer cat(cfg);
  std::vector<ScanEntry> out;
  cat.classifyFile("/home/user/.cache/thumbnails/foo.png", 1024, nowUnix(), out);
  REQUIRE_FALSE(hasCategory(out, Category::Cache));
}

TEST_CASE("an unremarkable recent small file produces no entries", "[categorizer]") {
  Categorizer cat(baseConfig());
  std::vector<ScanEntry> out;
  cat.classifyFile("/home/user/project/notes.txt", 100, nowUnix(), out);
  REQUIRE(out.empty());
}

TEST_CASE("finalizeDuplicates groups same-size same-name files when enabled", "[categorizer]") {
  auto cfg = baseConfig();
  cfg.scan_duplicates = true;
  Categorizer cat(cfg);

  std::vector<ScanEntry> entries;
  ScanEntry a;
  a.path = "/home/user/a/report.pdf";
  a.size_bytes = 2048;
  entries.push_back(a);

  ScanEntry b;
  b.path = "/home/user/b/report.pdf";
  b.size_bytes = 2048;
  entries.push_back(b);

  cat.finalizeDuplicates(entries);

  int dup_count = 0;
  for (const auto& e : entries) {
    if (e.category == Category::Duplicate) ++dup_count;
  }
  REQUIRE(dup_count == 1);
}

TEST_CASE("finalizeDuplicates is a no-op when disabled", "[categorizer]") {
  Categorizer cat(baseConfig());  // scan_duplicates == false

  std::vector<ScanEntry> entries;
  ScanEntry a;
  a.path = "/home/user/a/report.pdf";
  a.size_bytes = 2048;
  entries.push_back(a);
  ScanEntry b;
  b.path = "/home/user/b/report.pdf";
  b.size_bytes = 2048;
  entries.push_back(b);

  cat.finalizeDuplicates(entries);

  REQUIRE(entries.size() == 2);
}

TEST_CASE("finalizeDuplicates ignores tiny files below the size floor", "[categorizer]") {
  auto cfg = baseConfig();
  cfg.scan_duplicates = true;
  Categorizer cat(cfg);

  std::vector<ScanEntry> entries;
  ScanEntry a;
  a.path = "/home/user/a/.gitkeep";
  a.size_bytes = 0;
  entries.push_back(a);
  ScanEntry b;
  b.path = "/home/user/b/.gitkeep";
  b.size_bytes = 0;
  entries.push_back(b);

  cat.finalizeDuplicates(entries);

  REQUIRE(entries.size() == 2);
}
