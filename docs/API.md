# API Reference

Base URL: `http://127.0.0.1:8765`

| Method | Path | Description |
|--------|------|-------------|
| GET | `/api/health` | Health check |
| GET | `/api/defaults` | Default scan configuration |
| POST | `/api/scan/start` | Begin scan |
| POST | `/api/scan/cancel` | Cancel scan |
| GET | `/api/scan/status` | Progress |
| GET | `/api/scan/results` | Filtered results |
| POST | `/api/delete` | Delete selected paths |

## Duplicate detection

`POST /api/scan/start` accepts `scan_duplicates` (default `false`) to enable
duplicate detection. By default this uses a fast size+filename heuristic:
files with the same size and the same base filename under different
directories are flagged as duplicates of the first one seen.

An additional opt-in flag, `hash_duplicates` (default `false`, only
consulted when `scan_duplicates` is also `true`), confirms each
size+filename candidate with a whole-file content hash before flagging it as
a duplicate. This is slower (every candidate file is read in full) but
avoids false positives from same-size/same-name-but-different-content files.
The size+name heuristic remains the default behavior; this is additive.

`/api/delete` also now requires every path in a delete request to either
match a path present in the results of the last scan, or fall under one of
the tool's default allow-listed scan roots — an arbitrary filesystem path
outside both is rejected.
