# Requirements Document

## Introduction

The Nepal-Grid Peak Load Controller is a smart gateway demand-response module designed for household-level load management in Nepal's constrained electrical grid. The system comprises three components: an ESP32-based firmware gateway that measures real-time household power draw via CT clamps (SCT-013) and performs deterministic appliance shedding; a high-concurrency Golang backend server that issues peak-stress instructions and aggregates telemetry; and a Flutter-based operator telemetry dashboard for real-time monitoring and control. The monorepo is organized into `/firmware`, `/backend`, and `/app` directories with production-ready, fully documented boilerplate code.

## Glossary

- **Gateway**: The ESP32-WROOM-32 microcontroller module running native ESP-IDF firmware that performs sensor reading, load priority evaluation, and appliance shedding.
- **CT_Clamp**: A non-invasive current transformer (SCT-013) used to measure household power draw without interrupting the circuit.
- **Sensor_Task**: The FreeRTOS task (`Task_ReadSensors`) responsible for high-frequency ADC sampling of the CT clamp signal, pinned to Core 1.
- **Network_Task**: The FreeRTOS task (`Task_NetworkSync`) responsible for backend communication and telemetry synchronization, pinned to Core 0.
- **Smoothing_Window**: A 3-second rolling average buffer used to suppress inrush current spikes from motor-driven appliances.
- **Backend_Server**: The Golang HTTP server implementing Clean Architecture/DDD patterns that manages peak-stress signaling and telemetry ingestion.
- **Dashboard**: The Flutter-based operator telemetry application providing real-time visualization of grid load and appliance states.
- **Peak_Stress_Instruction**: A command issued by the Backend_Server to the Gateway indicating that the grid is under peak load and appliance shedding must commence.
- **Appliance_Shedding**: The process of disconnecting non-critical appliances in priority order to reduce total household power draw below a threshold.
- **Graceful_Teardown**: The orderly shutdown sequence of the Backend_Server capturing OS interrupt signals and draining active connections before termination.
- **Build_System**: The CMake-based ESP-IDF build toolchain used to compile and flash firmware to the ESP32 target.
- **ADC_Oneshot**: The modern ESP-IDF ADC abstraction (`esp_adc/adc_oneshot.h`) used for single-shot analog-to-digital conversion readings.

## Requirements

### Requirement 1: Firmware Build System Configuration

**User Story:** As a firmware developer, I want a properly configured ESP-IDF CMake build system, so that I can compile and flash the Gateway firmware using standard ESP-IDF tooling without Arduino or PlatformIO dependencies.

#### Acceptance Criteria

1. THE Build_System SHALL declare a minimum CMake version of 3.16 in the root `/firmware/CMakeLists.txt` file.
2. THE Build_System SHALL include the ESP-IDF CMake toolchain file (`$ENV{IDF_PATH}/tools/cmake/project.cmake`) and call the `project()` macro with a defined project name in the root `/firmware/CMakeLists.txt` file.
3. THE Build_System SHALL register the main component using `idf_component_register()` in `/firmware/main/CMakeLists.txt` with explicit source file declarations (no GLOB patterns) and a REQUIRES list specifying all dependent ESP-IDF components.
4. THE Build_System SHALL target the ESP32-WROOM-32 microcontroller by providing an `sdkconfig.defaults` file in `/firmware/` that sets the IDF target to `esp32`, using exclusively native ESP-IDF v5.0 or higher APIs.
5. THE Build_System SHALL contain no references to Arduino core libraries (e.g., `Arduino.h`), PlatformIO configuration files (e.g., `platformio.ini`), or framework wrapper libraries that replace native ESP-IDF APIs.
6. WHEN a developer runs `idf.py build` from the `/firmware` directory with ESP-IDF v5.x installed, THE Build_System SHALL produce a flashable binary without errors.

### Requirement 2: Firmware Sensor Sampling Task

**User Story:** As a firmware developer, I want a deterministic FreeRTOS task for CT clamp sampling, so that the Gateway can measure household power draw at high frequency without interfering with network operations.

#### Acceptance Criteria

1. THE Gateway SHALL initialize the Sensor_Task using `xTaskCreatePinnedToCore` with priority level 2, a stack size of 4096 bytes, and pinned to Core 1.
2. THE Sensor_Task SHALL configure the ADC channel using the ADC_Oneshot driver abstraction (`esp_adc/adc_oneshot.h`) with 12-bit resolution and ADC_ATTEN_DB_11 attenuation.
3. THE Sensor_Task SHALL maintain a Smoothing_Window variable representing a 3-second rolling average to suppress inrush current spikes.
4. THE Sensor_Task SHALL perform continuous ADC sampling in a loop using `vTaskDelay` with a sampling interval of 100 milliseconds.
5. WHEN a new ADC sample is acquired, THE Sensor_Task SHALL update the Smoothing_Window with the latest reading.
6. IF an ADC read operation fails, THEN THE Sensor_Task SHALL skip the failed sample without updating the Smoothing_Window and continue to the next sampling cycle.
7. THE Sensor_Task SHALL use the `app_main(void)` function as the firmware entry point for task initialization.

### Requirement 3: Firmware Network Synchronization Task

**User Story:** As a firmware developer, I want a dedicated FreeRTOS task for network communication, so that telemetry synchronization and backend polling operate independently from sensor sampling on a separate CPU core.

#### Acceptance Criteria

1. THE Gateway SHALL initialize the Network_Task using `xTaskCreatePinnedToCore` with priority level 1, a stack size of 4096 bytes, and pinned to Core 0.
2. THE Network_Task SHALL contain placeholder function calls representing the WebSocket or HTTP polling communication lifecycle with the Backend_Server, including connection initiation, data transmission, and response handling stages.
3. THE Network_Task SHALL operate in a continuous loop using `vTaskDelay` with a default polling interval of 10 seconds, defined as a compile-time constant via `#define`.
4. THE Network_Task SHALL remain decoupled from the Sensor_Task through separate core pinning (Core 0 vs Core 1) to prevent scheduling interference.
5. IF the Network_Task fails to communicate with the Backend_Server, THEN THE Network_Task SHALL continue operating its polling loop on the next interval without halting or restarting.
6. THE Gateway SHALL initialize the Network_Task from the `app_main(void)` entry point alongside the Sensor_Task.

### Requirement 4: Backend Server Initialization and Lifecycle

**User Story:** As a backend developer, I want a production-grade Go server with structured logging and graceful shutdown, so that the Backend_Server can handle high-concurrency connections and terminate cleanly under OS signals.

#### Acceptance Criteria

1. THE Backend_Server SHALL initialize using a `main.go` entry point located at `/cmd/api/main.go`.
2. THE Backend_Server SHALL use structured telemetry logging via the standard library `log/slog` package for all operational messages.
3. THE Backend_Server SHALL register signal handlers for `os.Interrupt` and `syscall.SIGTERM` to initiate Graceful_Teardown.
4. WHEN a termination signal is received, THE Backend_Server SHALL initiate Graceful_Teardown by draining active connections within a maximum timeout of 15 seconds before exiting.
5. IF active connections have not drained within the 15-second Graceful_Teardown timeout, THEN THE Backend_Server SHALL forcefully close remaining connections and exit with a non-zero exit code.
6. THE Backend_Server SHALL leverage native Go concurrency primitives (goroutines and channels) to manage incoming connections.
7. THE Backend_Server SHALL use Go modules (`go mod init`) for dependency management with a defined module path.
8. THE Backend_Server SHALL listen for incoming HTTP connections on a configurable TCP port with a default value of 8080.

### Requirement 5: Backend Clean Architecture Structure

**User Story:** As a backend developer, I want the server organized using Clean Architecture and DDD patterns, so that domain logic remains decoupled from infrastructure concerns and the codebase scales predictably.

#### Acceptance Criteria

1. THE Backend_Server SHALL organize source code into the following directory structure: `/cmd/api/`, `/internal/server/`, `/internal/domain/`, `/internal/repository/`, where each directory contains at least one `.go` source file declaring the corresponding package.
2. THE Backend_Server SHALL place domain entities and business rule interfaces exclusively within the `/internal/domain/` package, which SHALL NOT contain import statements referencing the `/internal/server/` or `/internal/repository/` packages.
3. THE Backend_Server SHALL place data persistence abstractions within the `/internal/repository/` package, which SHALL NOT contain import statements referencing the `/internal/server/` package.
4. THE Backend_Server SHALL place HTTP server configuration and routing within the `/internal/server/` package, which SHALL NOT contain import statements referencing the `/internal/repository/` package.
5. THE Backend_Server SHALL maintain unidirectional dependency flow such that the `/internal/domain/` package imports no other internal package, and the `/internal/server/` and `/internal/repository/` packages depend on `/internal/domain/` for shared type and interface definitions.
6. THE Backend_Server SHALL define repository contracts as Go interfaces within the `/internal/domain/` package, which the `/internal/repository/` package implements to satisfy Dependency Inversion.

### Requirement 6: Flutter Dashboard Application Structure

**User Story:** As a mobile developer, I want a Flutter application structured with feature-first clean architecture, so that the Dashboard can scale with additional features while maintaining separation of concerns.

#### Acceptance Criteria

1. THE Dashboard SHALL exist as a Flutter project within the `/app` directory with a valid `pubspec.yaml` file declaring the Flutter SDK dependency.
2. THE Dashboard SHALL organize source code using a feature-first architecture with `/lib/core/` for shared utilities and `/lib/features/dashboard/` for the telemetry dashboard feature, where `/lib/features/dashboard/` contains at minimum a `presentation/` subdirectory with a widget file serving as the feature's screen entry point.
3. THE Dashboard SHALL provide a `main.dart` entry point with an async `main` function that performs widget binding initialization followed by `runApp()` invocation, with no placeholder or stub logic.
4. THE Dashboard SHALL use `WidgetsFlutterBinding.ensureInitialized()` as the first statement in the `main` function before any asynchronous initialization operations.
5. THE Dashboard SHALL display a root `MaterialApp` widget with an application title matching the project name defined in `pubspec.yaml` and a `ThemeData` instance with an explicitly set `colorScheme`.
6. THE Dashboard SHALL contain at least one Dart file within `/lib/core/` that exports shared constants or utility definitions used by the application.
7. THE Dashboard SHALL use Riverpod 3 as the state management solution with `ProviderScope` wrapping the root `MaterialApp` widget in `main.dart`.
8. THE Dashboard SHALL use `@riverpod` code generation annotations (from `riverpod_annotation`) for all provider declarations, with generated code produced by `riverpod_generator` and `build_runner`.
9. THE Dashboard SHALL use `HookConsumerWidget` from `hooks_riverpod` as the base class for stateful feature screens that require both Riverpod provider access and hooks-based local state management.

### Requirement 7: Code Documentation and Production Readiness

**User Story:** As a developer, I want all generated code files to contain professional engineering documentation, so that architectural decisions are traceable and the codebase is immediately comprehensible to new contributors.

#### Acceptance Criteria

1. THE Gateway firmware source files SHALL contain file-level documentation comments explaining the architectural role of each module using C block comment syntax (`/** ... */`) at the top of each `.c` and `.h` file.
2. THE Backend_Server source files SHALL contain package-level documentation comments explaining the responsibility of each package using Go doc comment conventions (`// Package X ...`) in each package directory.
3. THE Dashboard source files SHALL contain file-level documentation comments explaining the purpose of each file using Dart doc comment syntax (`///`) at the top of each `.dart` file.
4. ALL hand-authored source files SHALL contain inline comments that explain the reasoning behind non-trivial implementation decisions (the "why", not the "what"), with at minimum one explanatory comment per function or logical block that involves domain-specific logic, hardware interaction, or architectural trade-offs.
5. ALL generated source files SHALL be syntactically valid and compile without errors using their respective toolchains: ESP-IDF v5.x CMake build for Gateway firmware, `go build ./...` for Backend_Server, and `flutter analyze` with zero errors for Dashboard.
6. ALL hand-authored source files SHALL contain no placeholder comments indicating unfinished implementation, including but not limited to patterns matching "TODO", "FIXME", "add logic here", "implement", or "placeholder", where the comment substitutes for actual implementation code.
7. THE documentation requirements in criteria 1 through 4 and criterion 6 SHALL apply only to hand-authored source files and SHALL NOT apply to tool-generated or auto-generated files such as build system outputs, plugin registrants, or lock files.
