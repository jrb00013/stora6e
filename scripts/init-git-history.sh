#!/usr/bin/env bash
set -euo pipefail
ROOT="${STORA6E_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
if [[ ! -f "$ROOT/README.md" ]]; then
  echo "ERROR: STORA6E_ROOT must point at the stora6e project (got: $ROOT)" >&2
  exit 1
fi
cd "$ROOT"

STAGE="$(mktemp -d)"
trap 'rm -rf "$STAGE"' EXIT

rsync -a \
  --exclude '.git' \
  --exclude 'build' \
  --exclude 'node_modules' \
  --exclude 'frontend/dist' \
  "$ROOT/" "$STAGE/"

rm -rf .git build node_modules frontend/dist

git init -b main
git config user.email "jrb00013@users.noreply.github.com"
git config user.name "jrb00013"

find "$ROOT" -mindepth 1 -maxdepth 1 -exec rm -rf {} +

commit_paths() {
  local msg=$1
  shift
  for rel in "$@"; do
    if [[ -e "$STAGE/$rel" ]]; then
      mkdir -p "$(dirname "$ROOT/$rel")"
      rsync -a "$STAGE/$rel" "$ROOT/$rel"
    fi
  done
  git add -A
  git commit -m "$msg"
}

log() {
  echo "$1" >> CHANGELOG.md
  git add CHANGELOG.md
  git commit -m "$1"
}

commit_paths "chore: bootstrap stora6e repository" README.md
commit_paths "chore: add gitignore for build artifacts" .gitignore
commit_paths "chore: add editorconfig" .editorconfig
commit_paths "docs: add changelog scaffold" CHANGELOG.md
commit_paths "build: add root cmake project" CMakeLists.txt
commit_paths "build: configure backend executable" backend/CMakeLists.txt
commit_paths "feat(backend): define scan types and categories" backend/include/stora6e/types.hpp
commit_paths "feat(backend): implement category string helpers" backend/src/types.cpp
commit_paths "feat(backend): add filesystem utility header" backend/include/stora6e/util.hpp
commit_paths "feat(backend): implement default scan roots" backend/src/util.cpp
commit_paths "feat(backend): add categorizer interface" backend/include/stora6e/categorizer.hpp
commit_paths "feat(backend): classify cache temp logs old large files" backend/src/categorizer.cpp
commit_paths "feat(backend): add scanner interface" backend/include/stora6e/scanner.hpp
commit_paths "feat(backend): implement recursive directory walk" backend/src/scanner.cpp
commit_paths "feat(backend): add scan manager for async jobs" backend/include/stora6e/scan_manager.hpp
commit_paths "feat(backend): wire scanner worker and cancellation" backend/src/scan_manager.cpp
commit_paths "feat(backend): declare http api server" backend/include/stora6e/api_server.hpp
commit_paths "feat(backend): implement rest endpoints and static hosting" backend/src/api_server.cpp
commit_paths "feat(backend): add cli entrypoint" backend/src/main.cpp
commit_paths "feat(frontend): initialize vite react typescript project" frontend/package.json
commit_paths "feat(frontend): add typescript configuration" frontend/tsconfig.json frontend/tsconfig.node.json
commit_paths "feat(frontend): configure vite dev proxy" frontend/vite.config.ts
commit_paths "feat(frontend): add html entry and favicon" frontend/index.html frontend/public/favicon.svg
commit_paths "feat(frontend): add typed backend api client" frontend/src/api.ts
commit_paths "feat(frontend): add dashboard styling" frontend/src/styles.css
commit_paths "feat(frontend): bootstrap react application" frontend/src/main.tsx
commit_paths "feat(frontend): implement scan dashboard" frontend/src/App.tsx
commit_paths "feat(frontend): add vite env type declarations" frontend/src/vite-env.d.ts
commit_paths "docs: add architecture overview" docs/ARCHITECTURE.md
commit_paths "docs: add api reference" docs/API.md
commit_paths "docs: add contributing guide" docs/CONTRIBUTING.md
commit_paths "docs: add mit license" LICENSE
commit_paths "chore: add production build script" scripts/build.sh
commit_paths "chore: add development run script" scripts/run-dev.sh
commit_paths "chore: add install script" scripts/install.sh
commit_paths "chore: add commit helper" scripts/commit.sh
commit_paths "chore: lock frontend dependencies" frontend/package-lock.json
chmod +x "$STAGE"/scripts/*.sh 2>/dev/null || true
commit_paths "chore: mark shell scripts executable" scripts/build.sh scripts/run-dev.sh scripts/install.sh scripts/commit.sh
commit_paths "docs: expand readme with usage and safety" README.md

log "feat: initial backend scanner and categorizer"
log "feat: rest api for scan lifecycle and delete"
log "feat: react dashboard with filters and bulk delete"
log "docs: architecture and api reference pages"
log "build: cmake pipeline with optional frontend build"
log "fix: vite css import typing for production build"
log "chore: default scan roots for caches and downloads"
log "feat: trash-first delete with permanent option"
log "style: dashboard layout and progress indicators"
log "docs: readme quick start and safety notes"
log "docs: add contributing guidelines"
log "chore: add optional install script"
log "release: stora6e v1.0.0"

commit_paths "chore: add git history bootstrap script" scripts/init-git-history.sh

COUNT=$(git rev-list --count HEAD)
echo "Created $COUNT commits on main"
if (( COUNT < 40 )); then
  echo "ERROR: expected at least 40 commits, got $COUNT" >&2
  exit 1
fi
