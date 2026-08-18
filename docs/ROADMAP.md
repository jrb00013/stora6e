# Roadmap

stora6e v1.0.0 is complete and functioning as intended: a local-only disk-cleanup
tool with a C++ backend and a React/Vite frontend. This is **not** a plan for a
rewrite or a new feature set — it's a short list of minor hardening and quality
items identified after the v1.0.0 release, tracked here so they don't get lost.

## Scope

This roadmap intentionally stays small. Anything not listed below is out of
scope for the current pass.

### Now

- **Harden `/api/delete`** — the backend is bound to `127.0.0.1` only, but the
  HTTP API itself trusted any `Origin` (wide-open CORS) and any filesystem
  path in the delete request body. Both are being tightened: CORS is
  restricted to a small allow-list of local dev origins, and delete requests
  must reference a path that was actually returned by a prior scan (or that
  falls under an allow-listed root), not an arbitrary path on disk.
- **Unit test coverage** — `categorizer.cpp` (classification rules) and
  `scan_manager.cpp` (scan lifecycle/state transitions) currently have zero
  automated coverage. Adding a minimal Catch2-based test suite so future
  changes to classification rules or scan state handling can't silently
  regress.
- **Real `ScanStatus::Error` propagation** — the scanner currently swallows
  most `std::error_code` values from the filesystem walk. When a walk hits an
  unexpected failure, that should be visible as `ScanStatus::Error` with a
  message, not silently reported as `Complete`.
- **CI** — add a GitHub Actions workflow that builds the backend and runs the
  new test suite on every push/PR, so regressions are caught before merge.
- **Repo hygiene** — commit the pending `scripts/install.sh` executable-bit
  fix (mode-only change, no content change).

### Later (optional, only if time allows)

- **Opt-in content-hash duplicate detection.** The existing duplicate
  heuristic (same size + same filename) stays the default — it's cheap and
  good enough for the common case. A `--hash-duplicates` / config flag can
  additionally enable a slower, exact content-hash comparison for users who
  want higher-confidence duplicate detection. This is additive only; the
  default behavior does not change.

## Explicitly out of scope

- No new categories, scan sources, or UI redesign.
- No auth/user system — this remains a single-user, localhost-only tool.
- No cross-platform (Windows/macOS) hardening beyond what already exists.
- No performance rework of the filesystem walker.
