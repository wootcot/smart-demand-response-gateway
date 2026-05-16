# Implementation Plan: Nepal Grid Peak Load Controller

## Overview

This plan implements a three-tier demand-response system across firmware (ESP32/ESP-IDF), backend (Go), and app (Flutter) components. Tasks are ordered by dependency: shared types and interfaces first, then core logic, then integration wiring. Each component is built feature-first with its own domain, data, and presentation layers.

## Tasks

- [x] 1. Firmware build system and core infrastructure
  - [x] 1.1 Create firmware CMake build system and sdkconfig
    - Create `/firmware/CMakeLists.txt` with minimum CMake 3.16, ESP-IDF toolchain include, and `project()` macro
    - Create `/firmware/sdkconfig.defaults` targeting ESP32 with IDF target set to `esp32`
    - Create `/firmware/main/CMakeLists.txt` with `idf_component_register()` using explicit source file list and REQUIRES for `driver`, `esp_adc`, `freertos`, `esp_http_client`, `log`
    - _Requirements: 1.1, 1.2, 1.3, 1.4, 1.5, 1.6_

  - [x] 1.2 Create firmware core shared state module
    - Create `/firmware/main/core/config.h` with compile-time constants: `SAMPLE_INTERVAL_MS` (100), `SMOOTHING_SAMPLES` (30), `NETWORK_POLL_INTERVAL_MS` (10000), ADC pin/channel definitions
    - Create `/firmware/main/core/shared_state.h` with `gateway_state_t` struct definition and accessor function declarations (`shared_state_init`, `shared_state_write_sample`, `shared_state_read_average`)
    - Create `/firmware/main/core/shared_state.c` implementing mutex-protected state initialization, write, and read operations using FreeRTOS `xSemaphoreCreateMutex` and `xSemaphoreTake`/`xSemaphoreGive`
    - _Requirements: 2.3, 2.5, 3.4_

  - [x] 1.3 Create firmware sensor feature module
    - Create `/firmware/main/features/sensor/smoothing.h` declaring `smoothing_update()` and `smoothing_get_average()` pure computation functions
    - Create `/firmware/main/features/sensor/smoothing.c` implementing 3-second rolling average (30 samples), buffer wraparound, and failed-read skipping logic
    - Create `/firmware/main/features/sensor/sensor_task.h` declaring the sensor task public API
    - Create `/firmware/main/features/sensor/sensor_task.c` implementing `Task_ReadSensors`: ADC oneshot driver configuration (12-bit, ADC_ATTEN_DB_11), 100ms sampling loop with `vTaskDelay`, smoothing window update, mutex-protected shared state write
    - _Requirements: 2.1, 2.2, 2.3, 2.4, 2.5, 2.6_

  - [x]\* 1.4 Write property test for smoothing window correctness
    - **Property 1: Smoothing Window Correctness**
    - Use `theft` C property-based testing library to generate random ADC reading sequences (mix of successful reads and failures)
    - Verify that `current_avg_watts` equals arithmetic mean of last `min(successful_count, 30)` successful readings
    - Verify failed reads do not alter buffer contents or average
    - Minimum 100 iterations
    - **Validates: Requirements 2.3, 2.5, 2.6**

  - [x] 1.5 Create firmware network feature module
    - Create `/firmware/main/features/network/network_task.h` declaring the network task public API
    - Create `/firmware/main/features/network/network_task.c` implementing `Task_NetworkSync`: 10-second polling loop with `vTaskDelay`, mutex-protected shared state read, HTTP POST placeholder for telemetry transmission, response handling for peak-stress instructions, error logging via `ESP_LOGW` on communication failure
    - _Requirements: 3.1, 3.2, 3.3, 3.4, 3.5, 3.6_

  - [x] 1.6 Create firmware entry point (app_main)
    - Create `/firmware/main/main.c` implementing `app_main()`: call `shared_state_init()`, create Sensor Task pinned to Core 1 (priority 2, 4096B stack), create Network Task pinned to Core 0 (priority 1, 4096B stack) via `xTaskCreatePinnedToCore`
    - _Requirements: 2.1, 2.7, 3.1, 3.6_

- [x] 2. Checkpoint - Firmware compilation verification
  - Ensure all firmware source files compile without errors using `idf.py build`, ask the user if questions arise.

- [x] 3. Backend shared package and server infrastructure
  - [x] 3.1 Create Go module and shared types package
    - Create `/backend/go.mod` with module path and Go version
    - Create `/backend/internal/shared/types.go` with `GatewayID` type alias and timestamp helpers
    - Create `/backend/internal/shared/interfaces.go` with cross-feature contracts (minimal initially)
    - _Requirements: 4.7, 5.1, 5.5, 5.6_

  - [x] 3.2 Create shared middleware and HTTP server
    - Create `/backend/internal/shared/middleware.go` with logging middleware (wrapping `slog`), panic recovery middleware, and CORS middleware
    - Create `/backend/internal/shared/server.go` with HTTP server struct, configurable port (default 8080), router composition mounting feature handlers, and graceful shutdown logic with context cancellation and 15-second drain timeout
    - _Requirements: 4.2, 4.3, 4.4, 4.5, 4.6, 4.8_

  - [x]\* 3.3 Write property test for graceful shutdown
    - **Property 2: Graceful Shutdown Drains Active Connections**
    - Use `rapid` Go property-based testing library to generate random sets of active HTTP connections with response times < 15 seconds
    - Verify all connections complete before server exits when termination signal is received
    - Verify total shutdown duration does not exceed 15 seconds
    - Minimum 100 iterations
    - **Validates: Requirements 4.4, 4.5**

- [x] 4. Backend feature modules
  - [x] 4.1 Implement telemetry feature package
    - Create `/backend/internal/features/telemetry/domain.go` with `TelemetryReading` entity struct (GatewayID, PowerWatts, Timestamp)
    - Create `/backend/internal/features/telemetry/repository.go` with `TelemetryRepository` interface declaring `Store` and `GetLatest` methods
    - Create `/backend/internal/features/telemetry/memory_store.go` implementing `TelemetryRepository` with in-memory storage (sync.RWMutex-protected map)
    - Create `/backend/internal/features/telemetry/handler.go` with `POST /telemetry` HTTP handler: request validation, JSON deserialization into `TelemetryRequest`, repository store call, return `TelemetryResponse` with `peak_stress_active` flag
    - _Requirements: 5.1, 5.2, 5.5, 5.6_

  - [x] 4.2 Implement gateway feature package
    - Create `/backend/internal/features/gateway/domain.go` with `GatewayStatus` entity struct (GatewayID, LastSeen, LastPowerWatts, PeakStressActive)
    - Create `/backend/internal/features/gateway/repository.go` with `GatewayRepository` interface declaring `UpdateStatus` and `GetAll` methods
    - Create `/backend/internal/features/gateway/memory_store.go` implementing `GatewayRepository` with in-memory storage
    - Create `/backend/internal/features/gateway/handler.go` with `GET /status` HTTP handler: aggregate gateway statuses, compute total load, return `StatusResponse`
    - _Requirements: 5.1, 5.3, 5.5, 5.6_

  - [x]\* 4.3 Write property test for feature package dependency isolation
    - **Property 3: Feature Package Dependency Isolation**
    - Use `rapid` Go property-based testing library to scan all Go source files within `/internal/features/telemetry/` and `/internal/features/gateway/`
    - Verify no import statements reference the other feature package
    - Verify each feature only imports from `/internal/shared/` and standard library
    - Minimum 100 iterations
    - **Validates: Requirements 5.2, 5.3, 5.4, 5.5**

  - [x] 4.4 Create backend composition root (main.go)
    - Create `/backend/cmd/api/main.go`: configure `slog` structured logger, instantiate feature repositories, instantiate feature handlers with injected repositories, compose HTTP router from shared middleware + feature handlers, register OS signal handlers (`os.Interrupt`, `syscall.SIGTERM`), initiate graceful shutdown
    - _Requirements: 4.1, 4.2, 4.3, 4.6, 4.7_

- [x] 5. Checkpoint - Backend compilation and tests
  - Ensure all backend code compiles with `go build ./...` and tests pass with `go test ./...`, ask the user if questions arise.

- [x] 6. Flutter dashboard application
  - [x] 6.1 Configure Flutter project and dependencies
    - Update `/app/pubspec.yaml` to add runtime dependencies: `flutter_riverpod`, `riverpod_annotation`, `hooks_riverpod`, `flutter_hooks`, `http`
    - Add dev dependencies: `riverpod_generator`, `build_runner`, `riverpod_lint`
    - _Requirements: 6.1, 6.7, 6.8_

  - [x] 6.2 Create core constants and shared utilities
    - Create `/app/lib/core/constants.dart` with API base URL, polling intervals, and color palette tokens
    - _Requirements: 6.6_

  - [x] 6.3 Create dashboard domain models
    - Create `/app/lib/features/dashboard/domain/models.dart` with `TelemetryReading`, `GatewayStatus`, and `GridStatus` data classes including `fromJson` factory constructors
    - _Requirements: 6.2_

  - [x] 6.4 Create dashboard data repository
    - Create `/app/lib/features/dashboard/data/dashboard_repository.dart` with HTTP client for backend communication, JSON deserialization into domain models, `fetchGridStatus()` method
    - _Requirements: 6.2_

  - [x] 6.5 Create dashboard Riverpod providers with code generation
    - Create `/app/lib/features/dashboard/providers/dashboard_providers.dart` with `@riverpod` annotated `dashboardRepository` and `gridStatus` async providers
    - Run `dart run build_runner build` to generate `dashboard_providers.g.dart`
    - _Requirements: 6.7, 6.8_

  - [x] 6.6 Create dashboard presentation screen
    - Create `/app/lib/features/dashboard/presentation/dashboard_screen.dart` as `HookConsumerWidget` displaying grid load status, connected gateways list, and peak stress indicator
    - Use `ref.watch()` for reactive provider access and `useEffect`/`useState` hooks for local state
    - _Requirements: 6.2, 6.5, 6.9_

  - [x] 6.7 Create Flutter app entry point (main.dart)
    - Update `/app/lib/main.dart` with async `main` function: `WidgetsFlutterBinding.ensureInitialized()` first, `runApp()` wrapping `MaterialApp` in `ProviderScope`, explicit `ThemeData` with `ColorScheme`, route to `DashboardScreen`
    - _Requirements: 6.3, 6.4, 6.5, 6.7_

- [x] 7. Checkpoint - Flutter analysis
  - Ensure `flutter analyze` reports zero errors, ask the user if questions arise.

- [x] 8. Documentation and production readiness
  - [x] 8.1 Add file-level documentation to all firmware source files
    - Add `/** ... */` block comments at the top of each `.c` and `.h` file explaining architectural role
    - Add inline comments explaining non-trivial implementation decisions (the "why") for domain-specific logic and hardware interactions
    - _Requirements: 7.1, 7.4, 7.7_

  - [x] 8.2 Add package-level documentation to all backend source files
    - Add `// Package X ...` doc comments in each Go package file explaining package responsibility
    - Add inline comments explaining non-trivial implementation decisions for domain logic and architectural trade-offs
    - _Requirements: 7.2, 7.4, 7.7_

  - [x] 8.3 Add file-level documentation to all Flutter source files
    - Add `///` doc comments at the top of each `.dart` file explaining file purpose
    - Add inline comments explaining non-trivial implementation decisions for state management and architectural choices
    - _Requirements: 7.3, 7.4, 7.7_

  - [x]\* 8.4 Write property test for source file documentation presence
    - **Property 4: Source File Documentation Presence**
    - Use `glados` Dart property-based testing library to scan all hand-authored source files
    - Verify `.c`/`.h` files contain `/** ... */` block comments
    - Verify Go package files contain `// Package ...` doc comments
    - Verify `.dart` files contain `///` doc comments
    - Minimum 100 iterations
    - **Validates: Requirements 7.1, 7.2, 7.3**

  - [x]\* 8.5 Write property test for no placeholder comments
    - **Property 5: No Placeholder Comments in Production Code**
    - Use `glados` Dart property-based testing library to scan all hand-authored source files
    - Verify no comment patterns matching "TODO", "FIXME", "add logic here", "implement", or "placeholder" where the comment substitutes for actual implementation code
    - Minimum 100 iterations
    - **Validates: Requirements 7.6**

- [x] 9. Final checkpoint - Full build verification
  - Ensure all components build successfully (`idf.py build`, `go build ./...`, `flutter analyze`), all tests pass, ask the user if questions arise.

## Notes

- Tasks marked with `*` are optional and can be skipped for faster MVP
- Each task references specific requirements for traceability
- Checkpoints ensure incremental validation across all three components
- Property tests validate universal correctness properties from the design document
- The firmware uses native ESP-IDF v5.x APIs exclusively (no Arduino/PlatformIO)
- Backend uses feature-first Clean Architecture with in-memory repositories (upgradeable to PostgreSQL)
- Flutter uses Riverpod 3 code generation + flutter_hooks for compile-time safe state management

## Task Dependency Graph

```json
{
  "waves": [
    { "id": 0, "tasks": ["1.1", "3.1", "6.1"] },
    { "id": 1, "tasks": ["1.2", "3.2", "6.2", "6.3"] },
    { "id": 2, "tasks": ["1.3", "1.5", "3.3", "4.1", "4.2", "6.4"] },
    { "id": 3, "tasks": ["1.4", "1.6", "4.3", "4.4", "6.5"] },
    { "id": 4, "tasks": ["6.6"] },
    { "id": 5, "tasks": ["6.7"] },
    { "id": 6, "tasks": ["8.1", "8.2", "8.3"] },
    { "id": 7, "tasks": ["8.4", "8.5"] }
  ]
}
```
