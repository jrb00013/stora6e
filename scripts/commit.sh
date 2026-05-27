#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

if [[ ! -d .git ]]; then
  echo "Run scripts/init-git-history.sh first"
  exit 1
fi

git add -A
if git diff --cached --quiet; then
  echo "Nothing to commit"
  exit 0
fi

git commit -m "${1:-chore: update project files}"
