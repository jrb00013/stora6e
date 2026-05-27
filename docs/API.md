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
