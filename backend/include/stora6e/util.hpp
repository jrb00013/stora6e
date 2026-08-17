#pragma once

#include <cstdint>
#include <string>
#include <system_error>
#include <vector>

namespace stora6e {

std::string homeDirectory();
std::vector<std::string> defaultScanRoots();
bool shouldSkipPath(const std::string& path);
std::string formatBytes(std::uint64_t bytes);
std::int64_t nowUnix();
std::string urlDecode(const std::string& value);

// True for std::error_code values that are expected/benign during a
// best-effort filesystem walk of a live, unprivileged filesystem (the entry
// disappeared mid-walk, or we lack permission to read it) as opposed to an
// unexpected failure that should be surfaced as a real scan error.
bool isBenignFilesystemError(const std::error_code& ec);

}  // namespace stora6e
