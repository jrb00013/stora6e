#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace stora6e {

std::string homeDirectory();
std::vector<std::string> defaultScanRoots();
bool shouldSkipPath(const std::string& path);
std::string formatBytes(std::uint64_t bytes);
std::int64_t nowUnix();
std::string urlDecode(const std::string& value);

}  // namespace stora6e
