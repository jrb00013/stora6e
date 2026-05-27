#include "stora6e/categorizer.hpp"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <map>
#include <unordered_map>

#include "stora6e/util.hpp"

namespace stora6e {

namespace {

std::string lower(std::string s) {
  std::transform(s.begin(), s.end(), s.begin(),
                 [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return s;
}

bool pathContains(const std::string& path, const char* needle) {
  return lower(path).find(needle) != std::string::npos;
}

bool endsWith(const std::string& s, const char* suffix) {
  const auto n = ::strlen(suffix);
  return s.size() >= n && s.compare(s.size() - n, n, suffix) == 0;
}

}  // namespace

Categorizer::Categorizer(const ScanConfig& config) : config_(config) {
  old_cutoff_unix_ = nowUnix() - static_cast<std::int64_t>(config.min_age_days) * 86400;
}

void Categorizer::classifyFile(const std::string& path, std::uint64_t size,
                               std::int64_t mtime, std::vector<ScanEntry>& out) {
  const auto min_large = config_.min_size_mb * 1024ULL * 1024ULL;
  std::vector<Category> hits;

  if (config_.scan_cache &&
      (pathContains(path, "/.cache/") || pathContains(path, "/cache/") ||
       pathContains(path, "/_cacache/") || pathContains(path, "/.npm/") ||
       pathContains(path, "/.docker/") || pathContains(path, "/pip/cache"))) {
    hits.push_back(Category::Cache);
  }

  if (config_.scan_temp &&
      (pathContains(path, "/tmp/") || pathContains(path, "/temp/") ||
       endsWith(path, ".tmp") || endsWith(path, ".temp") || pathContains(path, "/.Trash/"))) {
    hits.push_back(Category::Temp);
  }

  if (config_.scan_logs &&
      (endsWith(path, ".log") || endsWith(path, ".log.1") || pathContains(path, "/logs/"))) {
    hits.push_back(Category::Log);
  }

  if (config_.scan_large && size >= min_large) {
    hits.push_back(Category::Large);
  }

  if (config_.scan_old && mtime > 0 && mtime < old_cutoff_unix_) {
    hits.push_back(Category::Old);
  }

  if (hits.empty()) return;

  for (Category cat : hits) {
    ScanEntry e;
    e.path = path;
    e.size_bytes = size;
    e.modified_unix = mtime;
    e.category = cat;
    e.selected_default = (cat == Category::Cache || cat == Category::Temp || cat == Category::Log);
    out.push_back(std::move(e));
  }
}

void Categorizer::finalizeDuplicates(std::vector<ScanEntry>& entries) {
  if (!config_.scan_duplicates) return;

  struct Key {
    std::uint64_t size;
    std::string name;
    bool operator<(const Key& o) const {
      return std::tie(size, name) < std::tie(o.size, o.name);
    }
  };

  std::map<Key, std::vector<std::string>> groups;
  for (const auto& e : entries) {
    if (e.category == Category::Duplicate) continue;
    const auto slash = e.path.find_last_of('/');
    const std::string name = slash == std::string::npos ? e.path : e.path.substr(slash + 1);
    groups[{e.size_bytes, name}].push_back(e.path);
  }

  for (const auto& [key, paths] : groups) {
    if (paths.size() < 2 || key.size < 1024) continue;
    for (std::size_t i = 1; i < paths.size(); ++i) {
      ScanEntry dup;
      dup.path = paths[i];
      dup.size_bytes = key.size;
      dup.category = Category::Duplicate;
      dup.detail = "duplicate of " + paths[0];
      dup.selected_default = true;
      entries.push_back(std::move(dup));
    }
  }
}

}  // namespace stora6e
