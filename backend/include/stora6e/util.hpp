#pragma once

#include <cstdint>
#include <optional>
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

// A cheap (non-cryptographic, FNV-1a) whole-file content hash, used to
// confirm same-size/same-name duplicate candidates when a scan opts into
// hash_duplicates. Returns nullopt if the file can't be read (e.g. it
// vanished or permissions changed between the walk and this call) — callers
// should treat that as "can't confirm" rather than "is a duplicate".
std::optional<std::uint64_t> fileContentHash(const std::string& path);

// True for std::error_code values that are expected/benign during a
// best-effort filesystem walk of a live, unprivileged filesystem (the entry
// disappeared mid-walk, or we lack permission to read it) as opposed to an
// unexpected failure that should be surfaced as a real scan error.
bool isBenignFilesystemError(const std::error_code& ec);

}  // namespace stora6e
