#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace stora6e {

enum class Category {
  Old,
  Large,
  Cache,
  Temp,
  Log,
  Duplicate,
  EmptyDir,
  Unknown
};

struct ScanEntry {
  std::string path;
  std::uint64_t size_bytes = 0;
  std::int64_t modified_unix = 0;
  Category category = Category::Unknown;
  std::string detail;
  bool selected_default = false;
};

struct ScanConfig {
  std::vector<std::string> roots;
  int min_age_days = 90;
  std::uint64_t min_size_mb = 50;
  bool scan_old = true;
  bool scan_large = true;
  bool scan_cache = true;
  bool scan_temp = true;
  bool scan_logs = true;
  bool scan_duplicates = false;
  bool scan_empty_dirs = false;
  int max_depth = 32;
  std::uint64_t max_results = 50000;
};

struct ScanProgress {
  std::string phase;
  std::string current_path;
  std::uint64_t files_scanned = 0;
  std::uint64_t dirs_scanned = 0;
  std::uint64_t bytes_scanned = 0;
  double percent = 0.0;
};

enum class ScanStatus { Idle, Running, Complete, Cancelled, Error };

std::string categoryToString(Category c);
Category categoryFromString(const std::string& s);

}  // namespace stora6e
