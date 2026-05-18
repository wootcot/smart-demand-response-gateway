# Backend — Smart Demand Response Gateway

Go backend for the Nepal Grid Peak Load Controller. It receives real-time power telemetry from ESP32 gateways, tracks gateway status, and pushes peak-stress load-shedding commands over WebSocket connections.

## Architecture

The project follows Clean Architecture with feature-based packaging. Each feature owns its domain types, repository interface, handler, and storage implementation. Features depend only on the `shared` package and the standard library — never on each other.

```
backend/
├── cmd/api/main.go            # Composition root — wires dependencies and starts the server
├── internal/
│   ├── features/
│   │   ├── telemetry/         # Power reading ingestion (POST /telemetry)
│   │   ├── gateway/           # Gateway status aggregation (GET /status)
│   │   └── websocket/         # Full-duplex WebSocket channel (GET /ws)
│   └── shared/                # Common types, server, and middleware
├── go.mod
├── Makefile
└── .air.toml                  # Hot-reload config for development
```

## API Endpoints

| Method | Path         | Description                                            |
| ------ | ------------ | ------------------------------------------------------ |
| POST   | `/telemetry` | Ingest a power reading from a gateway                  |
| GET    | `/status`    | Retrieve all gateway statuses and total grid load      |
| GET    | `/ws`        | WebSocket upgrade for persistent gateway communication |

### POST /telemetry

Accepts a JSON body with a power measurement from a gateway's CT clamp sensor.

**Request:**

```json
{
  "gateway_id": "esp32-gw-001",
  "power_watts": 1450.5
}
```

**Response (200):**

```json
{
  "acknowledged": true,
  "peak_stress_active": false
}
```

### GET /status

Returns an aggregated view of all connected gateways for the Flutter dashboard.

**Response (200):**

```json
{
  "gateways": [
    {
      "gateway_id": "esp32-gw-001",
      "last_seen": "2026-05-18T10:30:00Z",
      "last_power_watts": 1450.5,
      "peak_stress_active": false
    }
  ],
  "total_load_watts": 1450.5,
  "peak_active": false
}
```

### GET /ws

Upgrades to a WebSocket connection. The gateway identifies itself via the `gateway_id` query parameter:

```
ws://localhost:8080/ws?gateway_id=esp32-gw-001
```

**Inbound frame (gateway → server):**

```json
{
  "type": "telemetry",
  "gateway_id": "esp32-gw-001",
  "power_watts": 1450.5
}
```

**Outbound frame (server → gateway):**

```json
{
  "type": "ack",
  "acknowledged": true,
  "peak_stress_active": false
}
```

**Peak-stress broadcast (server → all gateways):**

```json
{
  "type": "peak_stress",
  "peak_stress_active": true
}
```

## Middleware Stack

Applied in order (outermost first):

1. **Recovery** — catches panics, logs stack traces, returns 500
2. **Logging** — structured JSON logs for every request (method, path, status, duration)
3. **CORS** — permissive cross-origin headers for Flutter dashboard development

## Getting Started

### Prerequisites

- Go 1.23+
- [Air](https://github.com/air-verse/air) (optional, for hot-reload)

### Run

```bash
# Start the server on port 8080
make run

# Or with hot-reload (watches .go files, rebuilds on change)
make dev
```

### Build

```bash
make build
```

### Test

```bash
make test
```

### Lint

```bash
make lint
```

## Configuration

| Setting       | Default | Description                                 |
| ------------- | ------- | ------------------------------------------- |
| Port          | 8080    | TCP port the server listens on              |
| Drain timeout | 15s     | Max wait for in-flight requests on shutdown |

## Graceful Shutdown

The server listens for `SIGINT` and `SIGTERM`. On receipt it stops accepting new connections and waits up to 15 seconds for active requests to drain before forcing closure.

## Dependencies

| Package                                                    | Purpose                                     |
| ---------------------------------------------------------- | ------------------------------------------- |
| `github.com/coder/websocket`                               | WebSocket server implementation             |
| Standard library (`net/http`, `log/slog`, `encoding/json`) | HTTP server, structured logging, JSON codec |

## Storage

Currently uses in-memory stores for both telemetry readings and gateway status. The repository interfaces (`TelemetryRepository`, `GatewayRepository`) are designed for easy swap to a persistent backend (e.g., PostgreSQL) without changing handler logic.
