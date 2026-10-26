#include "stora6e/summary.hpp"

#include <unordered_map>

namespace stora6e {

namespace {

constexpr int categoryCount() {
  return static_cast<int>(Category::Unknown) + 1;
}

}  // namespace

EntrySummary summarizeEntries(const std::vector<ScanEntry>& entries) {
  EntrySummary summary;
  const int categories = categoryCount();
  summary.categories.resize(static_cast<std::size_t>(categories));
  for (int i = 0; i < categories; ++i) {
    summary.categories[static_cast<std::size_t>(i)].category = static_cast<Category>(i);
  }

  // First observed size wins. A path listed under several categories is the
  // same file, so later rows must not add its bytes again.
  std::unordered_map<std::string, std::uint64_t> unique_sizes;
  unique_sizes.reserve(entries.size());

  for (const auto& entry : entries) {
    const int idx = static_cast<int>(entry.category);
    if (idx >= 0 && idx < categories) {
      auto& tally = summary.categories[static_cast<std::size_t>(idx)];
      tally.count += 1;
      tally.bytes += entry.size_bytes;
    }
    summary.total_entries += 1;
    summary.total_bytes += entry.size_bytes;

    if (unique_sizes.find(entry.path) == unique_sizes.end()) {
      unique_sizes.emplace(entry.path, entry.size_bytes);
    }
  }

  summary.unique_paths = unique_sizes.size();
  for (const auto& item : unique_sizes) {
    summary.unique_bytes += item.second;
  }
  return summary;
}

}  // namespace stora6e
