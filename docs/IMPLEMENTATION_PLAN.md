# Implementation Plan

Ordered, commit-sized steps for the minor hardening pass described in
`docs/ROADMAP.md`. Each step is meant to be a single, coherent commit that
leaves the tree building and (once tests exist) passing.

1. **docs: add roadmap and implementation plan**
   This document plus `docs/ROADMAP.md`.

2. **fix: commit pending install.sh executable bit**
   Mode-only change (100644 → 100755), no content change. Kept as its own
   commit so it doesn't get buried in a larger diff.

3. **feat(security): restrict CORS and validate `/api/delete` paths**
   - Replace the wide-open `Access-Control-Allow-Origin: *` default header
     with a per-request check against a small allow-list of local origins
     (Vite dev server + the backend's own origin), reflected only when the
     request's `Origin` matches.
   - Add a `Host` header check (defends against DNS-rebinding attacks that
     rely on being able to reach `127.0.0.1` from an attacker-controlled
     hostname).
   - In the `/api/delete` handler, resolve each requested path with
     `weakly_canonical` and reject it unless it matches a path present in the
     last scan's results, or is nested under one of the default allow-listed
     scan roots.

4. **test: add Catch2 test scaffold and categorizer tests**
   - Vendor Catch2 via `FetchContent` in `backend/CMakeLists.txt`, gated
     behind a `STORA6E_BUILD_TESTS` option (default ON for local/CI builds).
   - Add `backend/tests/test_categorizer.cpp` covering the classification
     rules (old/large/cache/temp/log, multi-category hits, duplicate
     finalization).

5. **test: add scan_manager state-transition tests**
   - Add `backend/tests/test_scan_manager.cpp` covering
     Idle → Running → Complete, cancellation, and the "can't start while
     running" guard.

6. **fix(scanner): propagate real error state from the filesystem walk**
   - Track unexpected `std::error_code` failures (excluding expected/benign
     ones like permission-denied or the file having vanished mid-walk) during
     `Scanner::walk`.
   - Surface an aggregated error message via `Scanner::lastError()`.
   - `ScanManager::runWorker` sets `ScanStatus::Error` and populates
     `lastError()` when the scanner reports a walk error, while still keeping
     whatever partial results were gathered.
   - Add a regression test exercising this path.

7. **ci: add GitHub Actions workflow**
   - `.github/workflows/ci.yml`: configure, build the backend (tests
     included), and run `ctest` on push/PR to `main`.

8. **feat: optional content-hash duplicate detection (opt-in)**
   - Add a `hash_duplicates` config flag (default off) that, when enabled,
     confirms same-size/same-name duplicate candidates with a content hash
     before marking them as duplicates, instead of relying on size+name
     alone. Documented as additive in `docs/API.md`.

Steps 4–6 each rebuild and run the test suite before moving on; step 7 is
verified by confirming the workflow YAML is syntactically valid and mirrors
the local build/test commands.
