#include <catch2/catch_test_macros.hpp>

#include "stora6e/summary.hpp"

using namespace stora6e;

namespace {

const CategoryTotal& tally(const EntrySummary& summary, Category category) {
  return summary.categories.at(static_cast<std::size_t>(category));
}

ScanEntry entry(const std::string& path, std::uint64_t bytes, Category category) {
  ScanEntry e;
  e.path = path;
  e.size_bytes = bytes;
  e.category = category;
  return e;
}

}  // namespace

TEST_CASE("summarizeEntries on an empty scan is all zeros", "[summary]") {
  const auto summary = summarizeEntries({});

  REQUIRE(summary.total_entries == 0);
  REQUIRE(summary.total_bytes == 0);
  REQUIRE(summary.unique_paths == 0);
  REQUIRE(summary.unique_bytes == 0);
  REQUIRE(summary.categories.size() == static_cast<std::size_t>(Category::Unknown) + 1);
  for (const auto& category : summary.categories) {
    REQUIRE(category.count == 0);
    REQUIRE(category.bytes == 0);
  }
}

TEST_CASE("distinct paths add into per-category and unique totals", "[summary]") {
  const std::vector<ScanEntry> entries = {
      entry("/tmp/cache.bin", 1000, Category::Cache),
      entry("/tmp/old.log", 250, Category::Log),
  };

  const auto summary = summarizeEntries(entries);

  REQUIRE(tally(summary, Category::Cache).count == 1);
  REQUIRE(tally(summary, Category::Cache).bytes == 1000);
  REQUIRE(tally(summary, Category::Log).count == 1);
  REQUIRE(tally(summary, Category::Log).bytes == 250);
  REQUIRE(tally(summary, Category::Old).count == 0);
  REQUIRE(summary.total_entries == 2);
  REQUIRE(summary.total_bytes == 1250);
  REQUIRE(summary.unique_paths == 2);
  REQUIRE(summary.unique_bytes == 1250);
}

TEST_CASE("a path in two categories is counted once in the unique total", "[summary]") {
  const std::vector<ScanEntry> entries = {
      entry("/home/user/archive.tar", 4096, Category::Old),
      entry("/home/user/archive.tar", 4096, Category::Large),
      entry("/tmp/other.bin", 100, Category::Temp),
  };

  const auto summary = summarizeEntries(entries);

  REQUIRE(tally(summary, Category::Old).count == 1);
  REQUIRE(tally(summary, Category::Old).bytes == 4096);
  REQUIRE(tally(summary, Category::Large).count == 1);
  REQUIRE(tally(summary, Category::Large).bytes == 4096);
  REQUIRE(tally(summary, Category::Temp).count == 1);
  REQUIRE(tally(summary, Category::Temp).bytes == 100);

  // Per-category bytes overlap; the grand total must not.
  REQUIRE(summary.total_entries == 3);
  REQUIRE(summary.total_bytes == 8192 + 100);
  REQUIRE(summary.unique_paths == 2);
  REQUIRE(summary.unique_bytes == 4096 + 100);
}
