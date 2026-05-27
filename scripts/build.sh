#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="${ROOT}/build"

echo "==> Configuring stora6e"
cmake -S "$ROOT" -B "$BUILD_DIR" -DSTORA6E_BUILD_FRONTEND=ON

echo "==> Building"
cmake --build "$BUILD_DIR" -j"$(nproc)"

BIN="${BUILD_DIR}/backend/stora6e"
echo ""
echo "Built: $BIN"
echo "Run:   $BIN"
echo "UI:    http://127.0.0.1:8765"
