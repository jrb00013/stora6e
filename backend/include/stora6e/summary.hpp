#pragma once

#include <cstdint>
#include <vector>

#include "stora6e/types.hpp"

namespace stora6e {

// One category's slice of a scan. Counts and bytes are entry totals: a path
// that was classified into two categories contributes to both.
struct CategoryTotal {
  Category category = Category::Unknown;
  std::uint64_t count = 0;
  std::uint64_t bytes = 0;
};

// Summary of a scan result list.
//
// total_entries / total_bytes sum every ScanEntry, so a path that appears in
// more than one category is counted more than once. unique_paths /
// unique_bytes count each path a single time (the size from the first entry
// seen for that path) and are the reclaimable grand total.
struct EntrySummary {
  // One row per Category, in enum order, always present (zeros when empty).
  std::vector<CategoryTotal> categories;
  std::uint64_t total_entries = 0;
  std::uint64_t total_bytes = 0;
  std::uint64_t unique_paths = 0;
  std::uint64_t unique_bytes = 0;
};

EntrySummary summarizeEntries(const std::vector<ScanEntry>& entries);

}  // namespace stora6e
