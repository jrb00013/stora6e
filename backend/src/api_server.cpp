#include "stora6e/api_server.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <vector>

#include <httplib.h>
#include <nlohmann/json.hpp>

#include "stora6e/util.hpp"

namespace fs = std::filesystem;
using json = nlohmann::json;

namespace stora6e {

namespace {

// Local dev origins allowed to talk to the API. stora6e is a single-user,
// localhost-only tool; there is no reason for any other origin (or any
// non-localhost Host header) to reach it. Reflecting Origin unconditionally
// (the previous behavior) turns any page the user's browser visits into a
// potential drive-by trigger for /api/delete, and is also vulnerable to DNS
// rebinding since the server only checks the Host it thinks it's bound to
// implicitly. Both are checked explicitly below.
const std::vector<std::string>& allowedOrigins() {
  static const std::vector<std::string> origins = {
      "http://127.0.0.1:5173", "http://localhost:5173",
      "http://127.0.0.1:8765", "http://localhost:8765",
  };
  return origins;
}

bool isAllowedOrigin(const std::string& origin) {
  const auto& origins = allowedOrigins();
  return std::find(origins.begin(), origins.end(), origin) != origins.end();
}

bool isLocalHost(const std::string& host) {
  // Host header may or may not include the port.
  return host == "127.0.0.1" || host == "localhost" ||
         host.rfind("127.0.0.1:", 0) == 0 || host.rfind("localhost:", 0) == 0;
}

json entryToJson(const ScanEntry& e) {
  return json{{"path", e.path},
              {"size_bytes", e.size_bytes},
              {"size_human", formatBytes(e.size_bytes)},
              {"modified_unix", e.modified_unix},
              {"category", categoryToString(e.category)},
              {"detail", e.detail},
              {"selected_default", e.selected_default}};
}

ScanConfig parseConfig(const json& body) {
  ScanConfig cfg;
  if (body.contains("roots") && body["roots"].is_array()) {
    for (const auto& r : body["roots"]) {
      if (r.is_string()) cfg.roots.push_back(r.get<std::string>());
    }
  }
  if (cfg.roots.empty()) cfg.roots = defaultScanRoots();

  cfg.min_age_days = body.value("min_age_days", cfg.min_age_days);
  cfg.min_size_mb = body.value("min_size_mb", cfg.min_size_mb);
  cfg.scan_old = body.value("scan_old", cfg.scan_old);
  cfg.scan_large = body.value("scan_large", cfg.scan_large);
  cfg.scan_cache = body.value("scan_cache", cfg.scan_cache);
  cfg.scan_temp = body.value("scan_temp", cfg.scan_temp);
  cfg.scan_logs = body.value("scan_logs", cfg.scan_logs);
  cfg.scan_duplicates = body.value("scan_duplicates", cfg.scan_duplicates);
  cfg.hash_duplicates = body.value("hash_duplicates", cfg.hash_duplicates);
  cfg.scan_empty_dirs = body.value("scan_empty_dirs", cfg.scan_empty_dirs);
  cfg.max_depth = body.value("max_depth", cfg.max_depth);
  cfg.max_results = body.value("max_results", cfg.max_results);
  return cfg;
}

std::string statusString(ScanStatus s) {
  switch (s) {
    case ScanStatus::Idle:
      return "idle";
    case ScanStatus::Running:
      return "running";
    case ScanStatus::Complete:
      return "complete";
    case ScanStatus::Cancelled:
      return "cancelled";
    case ScanStatus::Error:
      return "error";
    default:
      return "unknown";
  }
}

// A delete request must reference a path that was actually surfaced by a
// prior scan, or that falls under one of the tool's own allow-listed default
// roots. This prevents an arbitrary-path delete via a crafted request body
// (e.g. "/etc/passwd" or "../../../etc") even if CORS/Host checks were
// somehow bypassed.
bool isAllowedDeletePath(const ScanManager& manager, const std::string& raw_path) {
  std::error_code ec;
  const fs::path canon = fs::weakly_canonical(raw_path, ec);
  if (ec) return false;
  const std::string resolved = canon.string();

  for (const auto& e : manager.results()) {
    std::error_code entry_ec;
    const fs::path entry_canon = fs::weakly_canonical(e.path, entry_ec);
    if (!entry_ec && entry_canon == canon) return true;
  }

  for (const auto& root : defaultScanRoots()) {
    std::error_code root_ec;
    const fs::path root_canon = fs::weakly_canonical(root, root_ec);
    if (root_ec) continue;
    const std::string rc = root_canon.string();
    if (resolved.rfind(rc, 0) != 0) continue;
    if (resolved.size() == rc.size() || resolved[rc.size()] == '/') return true;
  }

  return false;
}

bool deletePath(const std::string& path, bool use_trash, std::string& err) {
  std::error_code ec;
  if (!fs::exists(path, ec)) {
    err = "not found";
    return false;
  }

  if (use_trash) {
    const auto home = homeDirectory();
    const auto trash = home + "/.local/share/Trash/files";
    fs::create_directories(trash, ec);
    const auto base = fs::path(path).filename().string();
    auto dest = fs::path(trash) / base;
    int n = 1;
    while (fs::exists(dest, ec)) {
      dest = fs::path(trash) / (base + "." + std::to_string(n++));
    }
    fs::rename(path, dest, ec);
    if (!ec) return true;
    err = ec.message();
    return false;
  }

  if (fs::is_directory(path, ec)) {
    fs::remove_all(path, ec);
  } else {
    fs::remove(path, ec);
  }
  if (ec) {
    err = ec.message();
    return false;
  }
  return true;
}

}  // namespace

ApiServer::ApiServer(ScanManager& manager, const std::string& web_root, int port)
    : manager_(manager), web_root_(web_root), port_(port) {}

void ApiServer::run() {
  httplib::Server svr;

  // Reject anything that didn't come in over the loopback Host header before
  // it reaches any route. This is the DNS-rebinding defense: an attacker
  // page can get a browser to resolve evil.example to 127.0.0.1, but it
  // can't make the browser send a Host header of "127.0.0.1"/"localhost".
  svr.set_pre_routing_handler([](const httplib::Request& req, httplib::Response& res) {
    const auto host = req.get_header_value("Host");
    if (!host.empty() && !isLocalHost(host)) {
      res.status = 403;
      res.set_content(R"({"error":"forbidden host"})", "application/json");
      return httplib::Server::HandlerResponse::Handled;
    }

    const auto origin = req.get_header_value("Origin");
    if (!origin.empty() && isAllowedOrigin(origin)) {
      res.set_header("Access-Control-Allow-Origin", origin);
      res.set_header("Vary", "Origin");
      res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
      res.set_header("Access-Control-Allow-Headers", "Content-Type");
    }
    return httplib::Server::HandlerResponse::Unhandled;
  });

  svr.Options(R"(.*)", [](const httplib::Request&, httplib::Response& res) {
    res.status = 204;
  });

  svr.Get("/api/health", [](const httplib::Request&, httplib::Response& res) {
    res.set_content(json{{"ok", true}, {"app", "stora6e"}, {"version", "1.0.0"}}.dump(),
                    "application/json");
  });

  svr.Get("/api/defaults", [](const httplib::Request&, httplib::Response& res) {
    json roots = json::array();
    for (const auto& r : defaultScanRoots()) roots.push_back(r);
    res.set_content(json{{"roots", roots},
                         {"min_age_days", 90},
                         {"min_size_mb", 50},
                         {"categories", {"old", "large", "cache", "temp", "log", "duplicate"}}}
                        .dump(),
                    "application/json");
  });

  svr.Post("/api/scan/start", [this](const httplib::Request& req, httplib::Response& res) {
    json body = json::object();
    if (!req.body.empty()) {
      try {
        body = json::parse(req.body);
      } catch (...) {
        res.status = 400;
        res.set_content(R"({"error":"invalid json"})", "application/json");
        return;
      }
    }
    const auto cfg = parseConfig(body);
    if (!manager_.startScan(cfg)) {
      res.status = 409;
      res.set_content(R"({"error":"scan already running"})", "application/json");
      return;
    }
    res.set_content(R"({"ok":true})", "application/json");
  });

  svr.Post("/api/scan/cancel", [this](const httplib::Request&, httplib::Response& res) {
    manager_.cancelScan();
    res.set_content(R"({"ok":true})", "application/json");
  });

  svr.Get("/api/scan/status", [this](const httplib::Request&, httplib::Response& res) {
    const auto prog = manager_.progress();
  json out{{"status", statusString(manager_.status())},
             {"phase", prog.phase},
             {"current_path", prog.current_path},
             {"files_scanned", prog.files_scanned},
             {"dirs_scanned", prog.dirs_scanned},
             {"bytes_scanned", prog.bytes_scanned},
             {"bytes_scanned_human", formatBytes(prog.bytes_scanned)},
             {"percent", prog.percent}};
    if (auto err = manager_.lastError()) out["error"] = *err;
    res.set_content(out.dump(), "application/json");
  });

  svr.Get("/api/scan/results", [this](const httplib::Request& req, httplib::Response& res) {
    const auto results = manager_.results();
    const auto category = req.get_param_value("category");
    const auto q = req.get_param_value("q");
    const auto limit = std::stoul(req.get_param_value("limit").empty() ? "2000" : req.get_param_value("limit"));
    const auto offset = std::stoul(req.get_param_value("offset").empty() ? "0" : req.get_param_value("offset"));

    json items = json::array();
    std::uint64_t matched = 0;
    std::uint64_t skipped = 0;

    for (const auto& e : results) {
      if (!category.empty() && categoryToString(e.category) != category) continue;
      if (!q.empty() && e.path.find(q) == std::string::npos) continue;
      if (skipped < offset) {
        ++skipped;
        continue;
      }
      if (items.size() >= limit) {
        ++matched;
        continue;
      }
      items.push_back(entryToJson(e));
      ++matched;
    }

    const auto summary = manager_.summary();
    res.set_content(json{{"total", results.size()},
                         {"returned", items.size()},
                         {"items", items},
                         {"summary",
                          {{"total_reclaimable_bytes", summary.total_reclaimable_bytes},
                           {"total_reclaimable_human",
                            formatBytes(summary.total_reclaimable_bytes)}}}}
                        .dump(),
                    "application/json");
  });

  svr.Post("/api/delete", [this](const httplib::Request& req, httplib::Response& res) {
    json body;
    try {
      body = json::parse(req.body);
    } catch (...) {
      res.status = 400;
      res.set_content(R"({"error":"invalid json"})", "application/json");
      return;
    }

    if (!body.value("confirm", false)) {
      res.status = 400;
      res.set_content(R"({"error":"confirm must be true"})", "application/json");
      return;
    }

    const bool use_trash = body.value("use_trash", true);
    json paths = body.value("paths", json::array());
    json deleted = json::array();
    json failed = json::array();

    for (const auto& p : paths) {
      if (!p.is_string()) continue;
      const auto path = p.get<std::string>();

      if (!isAllowedDeletePath(manager_, path)) {
        failed.push_back(
            json{{"path", path}, {"error", "path is not part of a scan result or an allow-listed root"}});
        continue;
      }

      std::string err;
      if (deletePath(path, use_trash, err)) {
        deleted.push_back(path);
      } else {
        failed.push_back(json{{"path", path}, {"error", err}});
      }
    }

    res.set_content(json{{"deleted", deleted}, {"failed", failed}}.dump(), "application/json");
  });

  if (!web_root_.empty() && fs::is_directory(web_root_)) {
    if (!svr.set_mount_point("/", web_root_)) {
      // fallback manual static
    }
    svr.Get("/", [this](const httplib::Request&, httplib::Response& res) {
      const auto index = fs::path(web_root_) / "index.html";
      if (fs::exists(index)) {
        std::ifstream in(index);
        std::ostringstream ss;
        ss << in.rdbuf();
        res.set_content(ss.str(), "text/html");
      } else {
        res.set_content("<html><body><h1>stora6e</h1><p>Build the frontend.</p></body></html>",
                        "text/html");
      }
    });
  }

  std::cout << "stora6e listening on http://127.0.0.1:" << port_ << "\n";
  svr.listen("127.0.0.1", port_);
}

}  // namespace stora6e
