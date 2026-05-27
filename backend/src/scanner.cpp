#include "stora6e/scanner.hpp"

#include <filesystem>
#include <system_error>

#include "stora6e/util.hpp"

namespace fs = std::filesystem;

namespace stora6e {

Scanner::Scanner(ScanConfig config)
    : config_(std::move(config)), categorizer_(config_) {}

void Scanner::run(std::vector<ScanEntry>& results, ScanProgress& progress,
                  const std::atomic<bool>& cancel_flag,
                  std::function<void(const ScanProgress&)> on_progress) {
  results.clear();
  progress = ScanProgress{};
  progress.phase = "scanning";

  std::size_t root_count = config_.roots.size();
  if (root_count == 0) return;

  std::size_t root_index = 0;
  for (const auto& root : config_.roots) {
    if (cancel_flag.load()) break;
    progress.current_path = root;
    progress.percent = (100.0 * root_index) / static_cast<double>(root_count);
    if (on_progress) on_progress(progress);

    if (!fs::exists(root)) continue;
    walk(root, 0, results, progress, cancel_flag, on_progress);
    ++root_index;
  }

  progress.phase = "categorizing";
  progress.percent = 95.0;
  if (on_progress) on_progress(progress);

  categorizer_.finalizeDuplicates(results);

  progress.phase = "complete";
  progress.percent = 100.0;
  if (on_progress) on_progress(progress);
}

void Scanner::walk(const std::string& root, int depth, std::vector<ScanEntry>& results,
                   ScanProgress& progress, const std::atomic<bool>& cancel_flag,
                   std::function<void(const ScanProgress&)> on_progress) {
  if (cancel_flag.load()) return;
  if (depth > config_.max_depth) return;
  if (shouldSkipPath(root)) return;
  if (results.size() >= config_.max_results) return;

  std::error_code ec;
  const fs::path p(root);

  if (fs::is_symlink(p, ec)) return;

  if (fs::is_directory(p, ec)) {
    if (ec) return;
    ++progress.dirs_scanned;
    progress.current_path = root;

    if (config_.scan_empty_dirs) {
      bool empty = true;
      for (auto it = fs::directory_iterator(p, fs::directory_options::skip_permission_denied,
                                            ec);
           it != fs::directory_iterator(); it.increment(ec)) {
        if (ec) {
          ec.clear();
          continue;
        }
        empty = false;
        break;
      }
      if (empty && depth > 0) {
        ScanEntry e;
        e.path = root;
        e.category = Category::EmptyDir;
        e.selected_default = false;
        results.push_back(std::move(e));
      }
    }

    for (auto it = fs::directory_iterator(p, fs::directory_options::skip_permission_denied,
                                            ec);
         it != fs::directory_iterator(); it.increment(ec)) {
      if (cancel_flag.load() || results.size() >= config_.max_results) return;
      if (ec) {
        ec.clear();
        continue;
      }
      walk(it->path().string(), depth + 1, results, progress, cancel_flag, on_progress);
    }
    return;
  }

  if (!fs::is_regular_file(p, ec) || ec) return;

  const auto size = fs::file_size(p, ec);
  if (ec) return;

  std::int64_t mtime = 0;
  const auto ftime = fs::last_write_time(p, ec);
  if (!ec) {
    const auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
        ftime - fs::file_time_type::clock::now() + std::chrono::system_clock::now());
    mtime = std::chrono::duration_cast<std::chrono::seconds>(sctp.time_since_epoch()).count();
  }

  ++progress.files_scanned;
  progress.bytes_scanned += size;

  if (progress.files_scanned % 500 == 0 && on_progress) {
    on_progress(progress);
  }

  categorizer_.classifyFile(root, size, mtime, results);
}

}  // namespace stora6e
