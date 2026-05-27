#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
"$ROOT/scripts/build.sh"
install -m 755 "$ROOT/build/backend/stora6e" "${INSTALL_DIR:-/usr/local/bin}/stora6e"
echo "Installed stora6e to ${INSTALL_DIR:-/usr/local/bin}/stora6e"
