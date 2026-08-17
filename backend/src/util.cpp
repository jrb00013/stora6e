#include "stora6e/util.hpp"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <sstream>

namespace stora6e {

bool isBenignFilesystemError(const std::error_code& ec) {
  return ec == std::errc::permission_denied || ec == std::errc::no_such_file_or_directory;
}

std::string homeDirectory() {
  const char* home = std::getenv("HOME");
#ifdef _WIN32
  if (!home) home = std::getenv("USERPROFILE");
#endif
  return home ? std::string(home) : std::string("/");
}

std::vector<std::string> defaultScanRoots() {
  const auto home = homeDirectory();
  return {
      home + "/Downloads",
      home + "/.cache",
      home + "/.local/share/Trash",
      home + "/.npm/_cacache",
      home + "/.docker",
      "/tmp",
  };
}

bool shouldSkipPath(const std::string& path) {
  static const char* skip_prefixes[] = {
      "/proc", "/sys", "/dev", "/run", "/snap", "/boot", "/lost+found",
  };
  for (const auto* p : skip_prefixes) {
    if (path == p || path.rfind(p, 0) == 0) return true;
  }
  if (path.find("/.git/") != std::string::npos) return true;
  if (path.find("/node_modules/") != std::string::npos) return true;
  return false;
}

std::string formatBytes(std::uint64_t bytes) {
  const char* units[] = {"B", "KB", "MB", "GB", "TB"};
  double v = static_cast<double>(bytes);
  int u = 0;
  while (v >= 1024.0 && u < 4) {
    v /= 1024.0;
    ++u;
  }
  std::ostringstream oss;
  oss.setf(std::ios::fixed);
  oss.precision(u == 0 ? 0 : 2);
  oss << v << " " << units[u];
  return oss.str();
}

std::int64_t nowUnix() {
  return std::chrono::duration_cast<std::chrono::seconds>(
             std::chrono::system_clock::now().time_since_epoch())
      .count();
}

std::string urlDecode(const std::string& value) {
  std::string out;
  out.reserve(value.size());
  for (std::size_t i = 0; i < value.size(); ++i) {
    if (value[i] == '%' && i + 2 < value.size()) {
      const auto hex = value.substr(i + 1, 2);
      char* end = nullptr;
      const long c = std::strtol(hex.c_str(), &end, 16);
      if (end && *end == '\0') {
        out.push_back(static_cast<char>(c));
        i += 2;
        continue;
      }
    }
    if (value[i] == '+') {
      out.push_back(' ');
    } else {
      out.push_back(value[i]);
    }
  }
  return out;
}

}  // namespace stora6e
