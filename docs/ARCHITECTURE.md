# Architecture

stora6e runs entirely on localhost.

- **Backend (C++)** — filesystem scanner, categorizer, REST API via cpp-httplib
- **Frontend (React)** — Vite-built SPA served from the binary `web/` directory
- **Scan flow** — POST `/api/scan/start` → worker thread walk → GET `/api/scan/results`
