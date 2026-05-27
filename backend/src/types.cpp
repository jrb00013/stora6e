#include "stora6e/types.hpp"

namespace stora6e {

std::string categoryToString(Category c) {
  switch (c) {
    case Category::Old:
      return "old";
    case Category::Large:
      return "large";
    case Category::Cache:
      return "cache";
    case Category::Temp:
      return "temp";
    case Category::Log:
      return "log";
    case Category::Duplicate:
      return "duplicate";
    case Category::EmptyDir:
      return "empty_dir";
    default:
      return "unknown";
  }
}

Category categoryFromString(const std::string& s) {
  if (s == "old") return Category::Old;
  if (s == "large") return Category::Large;
  if (s == "cache") return Category::Cache;
  if (s == "temp") return Category::Temp;
  if (s == "log") return Category::Log;
  if (s == "duplicate") return Category::Duplicate;
  if (s == "empty_dir") return Category::EmptyDir;
  return Category::Unknown;
}

}  // namespace stora6e
