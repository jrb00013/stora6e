# stora6e

**stora6e** is a local storage cleanup app: a C++ backend scans your filesystem for reclaimable files (old, large, caches, temp, logs, duplicates), and a React web UI runs entirely on your machine.

Nothing is uploaded to the cloud. The API binds to `127.0.0.1` only.

## Features

- **Scoped scanning** — configurable roots (defaults: Downloads, `~/.cache`, Trash, npm/docker caches, `/tmp`)
- **Categories** — old files (by age), large files (by size), cache/temp/log paths, optional duplicate detection
- **Live progress** — files/dirs scanned, current path, byte count
- **Safe delete** — move to `~/.local/share/Trash` by default, or permanent delete with confirmation
- **Modern GUI** — dark-themed dashboard at `http://127.0.0.1:8765`

## Requirements

- Linux (primary target; macOS should work with minor path differences)
- CMake 3.16+, g++ with C++17
- Node.js 18+ and npm (for building the UI)

## Quick start

```bash
cd stora6e
chmod +x scripts/build.sh scripts/run-dev.sh
./scripts/build.sh
./build/backend/stora6e
```

Open http://127.0.0.1:8765 in your browser.

### Development (hot-reload UI)

```bash
./scripts/run-dev.sh
# UI: http://localhost:5173  (proxies API to :8765)
```

## CLI options

```
stora6e [--port 8765] [--web /path/to/dist] [--no-open]
```

Static files are served from `./web` next to the binary after a full build.

## API (local only)

| Method | Path | Description |
|--------|------|-------------|
| GET | `/api/health` | Health check |
| GET | `/api/defaults` | Default scan roots and thresholds |
| POST | `/api/scan/start` | Start scan (JSON body) |
| POST | `/api/scan/cancel` | Cancel running scan |
| GET | `/api/scan/status` | Progress |
| GET | `/api/scan/results` | Results (`?category=&q=&limit=&offset=`) |
| POST | `/api/delete` | Delete paths (`confirm: true`, `use_trash: true`) |

## Safety

- Review selections before deleting.
- Avoid scanning system roots (`/`, `/usr`) unless you know what you are doing.
- Duplicate detection uses size + filename grouping (fast heuristic, not full content hash).

## License

MIT
