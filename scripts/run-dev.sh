#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="${ROOT}/build"

cmake -S "$ROOT" -B "$BUILD_DIR" -DSTORA6E_BUILD_FRONTEND=OFF
cmake --build "$BUILD_DIR" -j"$(nproc)"

BIN="${BUILD_DIR}/backend/stora6e"

# Frontend dev server proxies /api to backend
(cd "$ROOT/frontend" && npm ci && npm run dev) &
FRONT_PID=$!

trap 'kill $FRONT_PID 2>/dev/null || true' EXIT

"$BIN" --port 8765 --no-open
