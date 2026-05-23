# Firmware Logic & Calculations

A developer-oriented guide to the calculations, algorithms, and programming techniques used in the Nepal Grid Peak Load Controller firmware (ESP32 / ESP-IDF / FreeRTOS).

---

## Table of Contents

1. [Architecture Overview](#architecture-overview)
2. [Configuration & Credential Management](#configuration--credential-management)
3. [True RMS Current Measurement](#true-rms-current-measurement)
4. [Two-Stage Smoothing Pipeline](#two-stage-smoothing-pipeline)
5. [ADS1115 ADC Configuration (Bit Manipulation)](#ads1115-adc-configuration)
6. [Relay Control & Inrush Protection](#relay-control--inrush-protection)
7. [Dual-Core Task Isolation](#dual-core-task-isolation)
8. [Mutex-Protected Shared State](#mutex-protected-shared-state)
9. [WebSocket Command Parsing (State Transitions)](#websocket-command-parsing)
10. [Fail-Safe Design Patterns](#fail-safe-design-patterns)
11. [C++ Tricks on a C API](#c-tricks-on-a-c-api)

---

## Architecture Overview

```
┌─────────────────────────────────────────────────────────────┐
│                        ESP32 Dual-Core                       │
│                                                             │
│  Core 1 (Priority 2)              Core 0 (Priority 1)      │
│  ┌─────────────────┐              ┌──────────────────┐     │
│  │  Sensor Task    │              │  Network Task    │     │
│  │  - I2C/ADS1115  │──── mutex ───│  - WebSocket TX  │     │
│  │  - RMS compute  │   (50ms     │  - Command RX    │     │
│  │  - Smoothing    │   timeout)   │  - Relay control │     │
│  └─────────────────┘              └──────────────────┘     │
└─────────────────────────────────────────────────────────────┘
```

Core separation is the foundational design decision. WiFi interrupts, TCP retransmits, and DNS lookups all happen on Core 0. The sensor sampling loop on Core 1 runs undisturbed with deterministic timing.

---

## Configuration & Credential Management

**Location:** `core/nvs_config.hpp`, `core/nvs_config.cpp`, `Kconfig.projbuild`

### The Problem

Hardcoding Wi-Fi credentials and backend URLs in source code creates security risks (secrets in git history) and deployment friction (must rebuild firmware for each device).

### Two-Tier Solution

```
┌─────────────────────────────────────────────────────────┐
│                   Configuration Lookup                    │
│                                                         │
│   NvsConfig::wifi_ssid()                                │
│       │                                                 │
│       ├── NVS has "wifi/ssid"? ──→ Return NVS value     │
│       │                                                 │
│       └── Not found? ──→ Return CONFIG_GATEWAY_WIFI_SSID│
│                          (Kconfig compile-time default)  │
└─────────────────────────────────────────────────────────┘
```

**Tier 1 — Kconfig (build-time):** Developers set credentials via `idf.py menuconfig`. Values are compiled into the binary as `CONFIG_GATEWAY_*` macros. The generated `sdkconfig` is gitignored; only `sdkconfig.defaults` (with safe placeholders) is committed.

**Tier 2 — NVS (runtime):** For production, per-device credentials are written to flash via NVS partition images. This allows flashing identical firmware binaries to multiple gateways while provisioning unique credentials without rebuilding.

### Implementation

```cpp
class NvsConfig {
    char ssid_[33] = {};
    bool ssid_loaded_ = false;
    // ...

public:
    bool load();  // Call after nvs_flash_init()

    const char* wifi_ssid() const {
        return ssid_loaded_ ? ssid_ : CONFIG_GATEWAY_WIFI_SSID;
    }
};
```

The `load()` method reads from two NVS namespaces:

| Namespace | Keys                          | Purpose                       |
| --------- | ----------------------------- | ----------------------------- |
| `wifi`    | `ssid`, `password`            | Wi-Fi AP credentials          |
| `gateway` | `ws_host`, `ws_port`, `gw_id` | Backend connection parameters |

### Boot Sequence

```cpp
// In app_main():
nvs_flash_init();       // Required by Wi-Fi driver anyway
nvs_config.load();      // Read runtime overrides (graceful if empty)

// Later, in network_client.init():
wifi_init_sta(config.wifi_ssid(), config.wifi_pass());  // NVS → Kconfig fallback
```

### Why Not Just NVS?

Pure NVS requires a provisioning step before first boot — the device won't connect out of the box. Kconfig defaults provide a working "development mode" configuration that requires zero provisioning, while NVS enables production-grade per-device customization.

### Provisioning Workflow (Production)

```bash
# 1. Create a CSV with device-specific credentials
echo "key,type,encoding,value
wifi,namespace,,
ssid,data,string,production_network
password,data,string,secure_password_here
gateway,namespace,,
ws_host,data,string,api.example.com
ws_port,data,u16,443
gw_id,data,string,gw-nepal-ktm-042" > nvs_data.csv

# 2. Generate binary partition image
python $IDF_PATH/components/nvs_flash/nvs_partition_gen/nvs_partition_gen.py \
    generate nvs_data.csv nvs.bin 0x6000

# 3. Flash to device
esptool.py write_flash 0x9000 nvs.bin
```

### What is NVS?

NVS (Non-Volatile Storage) is a key-value store built into a dedicated partition of the ESP32's onboard SPI flash. Think of it as a tiny embedded database soldered onto the chip.

Unlike a `.env` file in Node.js (which is a plain text file on a filesystem), NVS is:

- **Binary and wear-leveled** — Automatically rotates flash pages to prevent burning out cells from repeated writes
- **CRC-protected** — Detects corruption from power loss during writes
- **Partition-isolated** — Lives in its own flash partition, separate from firmware. Survives OTA firmware updates without being wiped
- **Runtime read/write** — Firmware can both read and write values at runtime (useful for provisioning flows over BLE or serial)

|                     | `.env` (Node.js)       | NVS (ESP32)                             |
| ------------------- | ---------------------- | --------------------------------------- |
| Storage medium      | File on disk/SSD       | Partition on SPI flash chip             |
| Format              | Plain text `KEY=value` | Binary, wear-leveled, CRC-protected     |
| Access              | `process.env.KEY`      | `nvs_get_str(handle, "key", buf, &len)` |
| Survives reboot     | Yes                    | Yes                                     |
| Survives re-flash   | No                     | **Yes** (separate partition)            |
| Editable at runtime | Not typically          | Yes                                     |
| Size                | Unlimited (disk space) | Typically 16–24 KB                      |

### Configuration Files & Git

| File                 | Contains                                                               | Committed to git?   | Node.js analogy            |
| -------------------- | ---------------------------------------------------------------------- | ------------------- | -------------------------- |
| `sdkconfig.defaults` | Safe placeholder values, project-wide build settings                   | **Yes**             | `.env.example`             |
| `sdkconfig`          | Your actual local config (real SSID/password after running menuconfig) | **No** (gitignored) | `.env`                     |
| `Kconfig.projbuild`  | Menu definitions (what options exist, types, help text)                | **Yes**             | Schema for `.env`          |
| NVS flash partition  | Per-device runtime credentials                                         | N/A (on-chip)       | Database / secrets manager |

When someone clones the repo and runs `idf.py build`, ESP-IDF generates `sdkconfig` by starting from `sdkconfig.defaults` and filling in anything missing with framework defaults. The developer then runs `idf.py menuconfig` to set their real credentials — which only lives in their local gitignored `sdkconfig`.

---

## True RMS Current Measurement

**Location:** `current_sensor.cpp → compute_rms_amps()`

### The Problem

A CT (current transformer) clamp outputs an AC signal centered around a DC bias. You can't just read the ADC and call it "current" — you need the root-mean-square value to get the equivalent DC heating value of the AC waveform.

### The Math

```
RMS = √( (1/N) × Σ(sample²) )
```

In code:

```cpp
// For each of ~430 samples over 500ms:
float voltage = raw_adc_count * 0.000125f;     // ADC count → volts (125µV/LSB)
float ac_voltage = voltage - 1.65f;            // subtract DC bias midpoint
float amps = ac_voltage * 30.0f;              // CT calibration: 30A per 1V
sum_of_squares += amps * amps;                // accumulate squared values

// After all samples:
float rms = sqrtf(sum_of_squares / valid_samples);
```

### Why 25 Cycles?

Nepal's grid runs at 50 Hz → one cycle = 20ms. We sample over 25 full cycles (500ms) so that:

- The RMS window is an exact integer multiple of the AC period (no partial-cycle bias)
- Short inrush spikes (1–5 cycles from motor startup) contribute only a fraction of their energy to the 25-cycle window, preventing false overload detection

### Why 860 SPS?

At 860 samples/second over 500ms, we get ~430 samples total → ~17 samples per AC cycle. The Nyquist criterion requires >2× the signal frequency, but for accurate RMS of a 50 Hz sine, 17 samples/cycle gives <0.5% error.

### LSB Voltage Derivation

```
PGA setting: ±4.096V full-scale
ADC resolution: 16-bit signed → 32768 positive counts
LSB = 4.096V / 32768 = 0.000125V = 125µV
```

### CT Calibration Factor

```
SCT-013-030: 30A primary → 1V secondary (across 33Ω burden)
So: amps = voltage_across_burden × 30
```

The DC bias midpoint (1.65V) comes from a 10kΩ + 10kΩ voltage divider on 3.3V, centering the AC signal in the ADC's input range.

---

## Two-Stage Smoothing Pipeline

**Location:** `current_sensor.cpp → read_smoothed_amps()`

### Stage 1: RMS Over 25 Cycles

Already described above. This inherently smooths out short transients because their energy is diluted across the full measurement window.

### Stage 2: Rolling Average (Circular Buffer)

```cpp
// Fixed-size circular buffer — no heap allocation
float smoothing_window[6] = {0};
uint8_t window_index = 0;

// Insert new RMS reading
smoothing_window[window_index] = rms_amps;
window_index = (window_index + 1) % SMOOTHING_WINDOW_SIZE;  // wraps at 6

// Compute arithmetic mean
float sum = 0.0f;
for (uint8_t i = 0; i < 6; i++) {
    sum += smoothing_window[i];
}
float smoothed = sum / 6.0f;
```

**Why 6 readings?** 6 × 500ms = 3 seconds. Motor inrush transients typically last 1–2 seconds. A 3-second rolling average ensures these don't trigger false peak-stress events.

**Why a circular buffer?** O(1) insertion, O(N) averaging with N=6 (trivial), zero heap allocation, and the oldest reading is naturally overwritten.

---

## ADS1115 ADC Configuration

**Location:** `config.h → ADS1115_CONFIG_CONTINUOUS`

The ADS1115 is configured via a 16-bit register where each bit field controls a different parameter. The firmware constructs this at compile time using bitwise operations:

```cpp
inline constexpr uint16_t ADS1115_CONFIG_CONTINUOUS =
    (0b0   << 15) |   // OS: no effect in continuous mode
    (0b100 << 12) |   // MUX: AIN0 vs GND (single-ended)
    (0b001 << 9)  |   // PGA: ±4.096V full-scale
    (0b0   << 8)  |   // MODE: continuous conversion
    (0b111 << 5)  |   // DR: 860 SPS (maximum rate)
    (0b0   << 4)  |   // COMP_MODE: traditional
    (0b0   << 3)  |   // COMP_POL: active low
    (0b0   << 2)  |   // COMP_LAT: non-latching
    (0b11  << 0);     // COMP_QUE: disable comparator
```

This is a common embedded pattern: build a hardware register value from named bit fields at compile time so there's zero runtime cost and the intent is self-documenting.

### GPIO Bitmask for Relay Pins

```cpp
io_conf.pin_bit_mask = (1ULL << RELAY_PIN_1) |
                       (1ULL << RELAY_PIN_2) |
                       (1ULL << RELAY_PIN_3);
```

ESP-IDF's `gpio_config()` accepts a 64-bit mask where each set bit represents a GPIO to configure. This configures all three relay pins in a single system call.

---

## Relay Control & Inrush Protection

**Location:** `relay_controller.cpp`

### Shedding (Disconnect Loads)

All relays are energized immediately in priority order (highest-priority load first). No delay needed — disconnecting loads doesn't cause inrush.

### Restoration (Reconnect Loads)

```cpp
// Restore in REVERSE priority order with staggered delay
for (int i = RELAY_COUNT - 1; i >= 0; i--) {
    gpio_set_level(pins[i], 0);       // de-energize relay
    if (i > 0) {
        vTaskDelay(pdMS_TO_TICKS(500)); // 500ms between reconnections
    }
}
```

**Why stagger?** Motor-driven appliances (pumps, compressors) draw 5–8× their rated current during startup. Reconnecting three motors simultaneously could trip the main breaker. The 500ms stagger ensures each motor's inrush has subsided before the next one starts.

**Why reverse order?** Lower-priority loads (EV charger, pump motor) are restored first, giving the highest-priority load (water heater) the most stable grid conditions when it reconnects.

---

## Dual-Core Task Isolation

**Location:** `main.cpp`

```cpp
// Sensor: Core 1, Priority 2 — deterministic ADC timing
xTaskCreatePinnedToCore(Task_ReadSensors, "SensorTask", 4096, nullptr, 2, nullptr, 1);

// Network: Core 0, Priority 1 — WiFi/WebSocket I/O
xTaskCreatePinnedToCore(Task_NetworkSync, "NetworkTask", 4096, nullptr, 1, nullptr, 0);
```

### Why This Matters

The ESP32's WiFi driver runs on Core 0 and generates frequent interrupts. If the sensor task were also on Core 0, those interrupts would introduce jitter into the ADC sampling timing, corrupting the RMS calculation.

By pinning the sensor task to Core 1 at a higher priority, we guarantee:

- No WiFi interrupt interference
- No preemption by the network task
- Deterministic ~1163µs inter-sample timing

### Microsecond-Precision Busy Wait

```cpp
esp_rom_delay_us(RMS_SAMPLE_INTERVAL_US);  // ~1163µs
```

This is a busy-wait (not `vTaskDelay`) because FreeRTOS tick resolution is typically 1ms, and we need sub-millisecond precision. The tradeoff is CPU usage during the 500ms burst, but since this task owns Core 1 entirely, there's nothing else to preempt.

---

## Mutex-Protected Shared State

**Location:** `gateway_state.cpp`

The sensor task (Core 1) writes power readings. The network task (Core 0) reads them. A FreeRTOS mutex synchronizes access:

```cpp
static constexpr TickType_t MUTEX_TIMEOUT = pdMS_TO_TICKS(50);

bool GatewayState::write_power(float watts) noexcept {
    if (xSemaphoreTake(mutex, MUTEX_TIMEOUT) != pdTRUE) {
        return false;  // skip this write rather than block indefinitely
    }
    current_avg_watts = watts;
    xSemaphoreGive(mutex);
    return true;
}
```

### The 50ms Timeout Trick

Instead of blocking forever (`portMAX_DELAY`), the mutex has a 50ms timeout. If contention occurs:

- **Sensor task** skips the write — the next reading arrives in 500ms anyway
- **Network task** returns 0.0 for power — the backend gets a zero and retries in 10s
- **Peak stress query** returns `false` — fail-safe keeps loads connected

This prevents a deadlock or priority inversion from stalling the real-time sampling loop.

---

## WebSocket Command Parsing

**Location:** `network_client.cpp → handle_message()`

### State-Transition Guard

```cpp
bool peak_active = (strnstr(data, "\"peak_stress_active\":true", len) != nullptr);
bool was_active = state.get_peak_stress();

state.set_peak_stress(peak_active);

// Only act on TRANSITIONS, not repeated states
if (peak_active && !was_active) {
    relays.shed_loads();
} else if (!peak_active && was_active) {
    relays.restore_loads();
}
```

**Why transitions only?** The backend might send the same command multiple times (network retries, duplicate messages). Without the transition guard, we'd call `shed_loads()` repeatedly — which is idempotent in this implementation, but the pattern prevents unnecessary GPIO writes and log spam.

### Lightweight JSON Parsing

Instead of pulling in a JSON library (cJSON adds ~30KB to flash), the firmware uses `strnstr()` to check for a known substring. This works because:

- The message format is fixed and controlled by our backend
- We only need to detect one boolean field
- It's O(n) with zero heap allocation

---

## Fail-Safe Design Patterns

### 1. Power-Loss Safe Relays

```
Active-HIGH logic: GPIO HIGH = relay energized = load DISCONNECTED
```

On MCU crash or power loss, all GPIOs go LOW → all relays de-energize → all loads reconnect. The system fails toward "everything connected" rather than "everything shed."

### 2. Graceful Degradation on Measurement Failure

```cpp
if (rms_amps < 0.0f) {
    // Return last known rolling average WITHOUT modifying the buffer
    float sum = 0.0f;
    for (uint8_t i = 0; i < SMOOTHING_WINDOW_SIZE; i++) {
        sum += smoothing_window[i];
    }
    return sum / SMOOTHING_WINDOW_SIZE;
}
```

If an I2C burst fails (>25% of samples lost), the smoothing buffer is left untouched and the last known average is returned. This prevents a single bad measurement from corrupting the rolling window.

### 3. 75% Validity Threshold

```cpp
uint16_t min_valid = (RMS_TOTAL_SAMPLES * 3) / 4;  // ~322 of ~430
if (valid_samples < min_valid) {
    return -1.0f;  // signal failure to caller
}
```

A few dropped I2C reads are fine — the RMS is still accurate with 75% of samples. But below that threshold, the statistical confidence drops and we discard the entire burst.

---

## C++ Tricks on a C API

### Static Trampoline Pattern

ESP-IDF's event system uses C function pointers. To dispatch into a C++ instance method:

```cpp
// Register with `this` as the opaque handler_args
esp_websocket_register_events(client, WEBSOCKET_EVENT_ANY, event_handler, this);

// Static method receives the pointer and casts back
static void event_handler(void *handler_args, ...) {
    auto *self = static_cast<NetworkClient *>(handler_args);
    // Now call instance methods on `self`
}
```

### Non-Copyable Resource Owners

```cpp
CurrentSensor(const CurrentSensor&) = delete;
CurrentSensor& operator=(const CurrentSensor&) = delete;
```

Each class owns a hardware resource (I2C bus, GPIO pins, WebSocket connection). Deleting copy/move operations makes it a compile error to accidentally duplicate ownership — the compiler enforces the hardware contract.

### Static Allocation (No Heap)

```cpp
static GatewayState gateway_state;
static CurrentSensor ct_sensor(CT_CALIBRATION_FACTOR);
static RelayController relay_controller;
static NvsConfig nvs_config;
static NetworkClient network_client(gateway_state, relay_controller, nvs_config);
```

All objects live in BSS/data segments with static lifetime. This eliminates:

- Heap fragmentation (critical on a 320KB RAM MCU)
- Allocation failure paths
- Constructor ordering issues (all initialized before `app_main`)

### Compile-Time Constants via `inline constexpr`

```cpp
inline constexpr uint16_t RMS_TOTAL_SAMPLES = (RMS_WINDOW_DURATION_MS * ADS1115_SPS) / 1000;
```

All configuration is resolved at compile time. No runtime computation, no RAM consumed for constants, and the compiler can optimize loops with known bounds.

---

## Summary of Key Numbers

| Parameter          | Value             | Rationale                                       |
| ------------------ | ----------------- | ----------------------------------------------- |
| ADC sample rate    | 860 SPS           | Max ADS1115 rate → ~17 samples/cycle at 50Hz    |
| RMS window         | 25 cycles (500ms) | Integer multiple of 20ms period, dilutes inrush |
| Smoothing window   | 6 readings (3s)   | Covers 1–2s motor inrush transients             |
| Mutex timeout      | 50ms              | Skip rather than block the sampling loop        |
| Telemetry interval | 10s               | Balances freshness vs. bandwidth                |
| Restore stagger    | 500ms             | Lets each motor's inrush subside                |
| WiFi retries       | 10                | Handles transient AP unavailability             |
| Validity threshold | 75% of samples    | Statistical confidence floor                    |
