#pragma once

#include <memory>
#include <string>

#include "stora6e/scan_manager.hpp"

namespace stora6e {

class ApiServer {
 public:
  ApiServer(ScanManager& manager, const std::string& web_root, int port);
  void run();
  int port() const { return port_; }

 private:
  ScanManager& manager_;
  std::string web_root_;
  int port_;
};

}  // namespace stora6e
