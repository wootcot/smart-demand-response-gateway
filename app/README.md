# App — Nepal Grid Peak Load Controller Dashboard

Flutter-based network operator telemetry dashboard for the Smart Demand Response Gateway. Provides real-time visualization of gateway power readings, grid load status, and peak-stress load-shedding events.

## Architecture

The app follows Clean Architecture with feature-based packaging and Riverpod for state management.

```
app/lib/
├── main.dart                          # Entry point — ProviderScope + MaterialApp
├── core/
│   ├── constants.dart                 # API URLs, polling intervals, color tokens
│   └── theme.dart                     # Material theme configuration
└── features/
    └── dashboard/
        ├── data/                      # Repository implementations, WebSocket client
        ├── domain/                    # Domain models, repository interfaces
        ├── presentation/             # DashboardScreen, widgets
        └── providers/                # Riverpod providers (code-generated)
```

## Features

- Real-time power telemetry display from connected ESP32 gateways
- WebSocket-driven live updates (no polling required for status changes)
- Visual grid status indicators: normal (green), elevated (amber), peak stress (red)
- Gateway online/offline status tracking
- Responsive to peak-stress events with faster refresh intervals (3s vs 10s)

## Getting Started

### Prerequisites

- Flutter SDK 3.11.5+
- Dart SDK 3.11.5+
- Running backend server (see `../backend/README.md`)

### Run

```bash
# Fetch dependencies
make get

# Run code generation (Riverpod providers)
make build-runner

# Launch in debug mode
make run
```

### Build

```bash
# Release APK
make build-apk
```

### Test & Lint

```bash
make test
make analyze
```

## Configuration

Backend connection is configured in `lib/core/constants.dart`:

| Setting                  | Default                            | Description                              |
| ------------------------ | ---------------------------------- | ---------------------------------------- |
| `apiBaseUrl`             | `http://localhost:8080`            | Backend REST API base URL                |
| `dashboardWsUrl`         | `ws://localhost:8080/ws/dashboard` | WebSocket endpoint for live grid updates |
| `gridStatusPollInterval` | 10s                                | Fallback polling interval                |
| `peakStressPollInterval` | 3s                                 | Faster polling during peak stress        |

## Dependencies

| Package               | Purpose                                 |
| --------------------- | --------------------------------------- |
| `hooks_riverpod`      | State management with hooks integration |
| `riverpod_annotation` | Code-generated providers                |
| `flutter_hooks`       | Lifecycle-aware widget hooks            |
| `web_socket_channel`  | WebSocket client for real-time updates  |
| `http`                | HTTP client for REST API calls          |
