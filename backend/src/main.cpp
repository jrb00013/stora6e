#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>

#include "stora6e/api_server.hpp"
#include "stora6e/scan_manager.hpp"

namespace fs = std::filesystem;

namespace {

void printUsage() {
  std::cout << "stora6e — local storage cleanup\n\n"
            << "Usage: stora6e [options]\n"
            << "  --port PORT       HTTP port (default 8765)\n"
            << "  --web PATH        Static web root (default: ./web next to binary)\n"
            << "  --no-open         Do not try to open browser\n"
            << "  -h, --help        Show help\n";
}

std::string exeDirectory(const char* argv0) {
  std::error_code ec;
  const auto p = fs::weakly_canonical(fs::path(argv0), ec);
  if (ec) return ".";
  if (p.has_parent_path()) return p.parent_path().string();
  return ".";
}

}  // namespace

int main(int argc, char* argv[]) {
  int port = 8765;
  std::string web_root;
  bool open_browser = true;

  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "-h" || arg == "--help") {
      printUsage();
      return 0;
    }
    if (arg == "--port" && i + 1 < argc) {
      port = std::atoi(argv[++i]);
    } else if (arg == "--web" && i + 1 < argc) {
      web_root = argv[++i];
    } else if (arg == "--no-open") {
      open_browser = false;
    }
  }

  if (web_root.empty()) {
    web_root = exeDirectory(argv[0]) + "/web";
  }

  stora6e::ScanManager manager;
  stora6e::ApiServer server(manager, web_root, port);

  if (open_browser) {
    const std::string url = "http://127.0.0.1:" + std::to_string(port);
#ifdef __linux__
    std::string cmd = "xdg-open '" + url + "' 2>/dev/null";
#elif __APPLE__
    std::string cmd = "open '" + url + "' 2>/dev/null";
#else
    std::string cmd;
#endif
    if (!cmd.empty()) {
      std::system(cmd.c_str());
    }
  }

  server.run();
  return 0;
}
