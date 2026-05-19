/**
 * @file config.h
 * @brief Compile-time constants for the Nepal Grid Peak Load Controller firmware.
 *
 * Centralizes all hardware pin definitions, sampling intervals, and system
 * timing parameters. These constants are shared across the sensor and network
 * feature modules to ensure consistent configuration without runtime overhead.
 *
 * ADC channel selection targets GPIO34 (ADC1_CHANNEL_6) which is an input-only
 * pin on ESP32-WROOM-32, chosen because it has no internal pull-up/pull-down
 * interference that could affect CT clamp signal accuracy.
 */

#pragma once

#include <cstdint>
#include "esp_adc/adc_oneshot.h"
#include "driver/gpio.h"  // provided by esp_driver_gpio component

// =============================================================================
// USER CONFIGURATION — Update these values for your deployment
// =============================================================================

// Wi-Fi credentials (move to NVS or Kconfig for production)
#define WIFI_SSID "YOUR_SSID"
#define WIFI_PASS "YOUR_PASSWORD"

// WebSocket backend URL (include gateway_id as query param)
#define WS_URL "ws://192.168.1.100:8080/ws?gateway_id=esp32-gw-001"

// =============================================================================
// SYSTEM CONFIGURATION — Typically no changes needed below this line
// =============================================================================

// --- ADC Configuration ---
// CT clamp (SCT-013) signal acquisition on GPIO34.
// ADC_ATTEN_DB_12 provides full-scale voltage range (~0-3.1V) needed to
// capture the CT clamp's output swing without clipping.
inline constexpr adc_unit_t     ADC_UNIT_ID   = ADC_UNIT_1;
inline constexpr adc_channel_t  ADC_CHAN       = ADC_CHANNEL_6;
inline constexpr adc_atten_t    ADC_ATTEN_LVL = ADC_ATTEN_DB_12;
inline constexpr adc_bitwidth_t ADC_BW        = ADC_BITWIDTH_12;

// TODO: Calibrate for actual SCT-013 variant and burden resistor.
// Current value (1.82) is a placeholder. Real calibration requires measuring
// known load current with a reference meter and adjusting this factor to match.
// Factor depends on: SCT-013 ratio (e.g. 30A/1V), burden resistor (e.g. 33Ω),
// voltage divider bias (mid-rail 1.65V), and ADC reference voltage.
inline constexpr float CT_CALIBRATION_FACTOR = 1.82f;

// --- Sampling Configuration ---
// 500ms interval between RMS measurement cycles. Each cycle performs a burst
// of rapid ADC samples spanning RMS_CYCLE_COUNT AC cycles, then sleeps until
// the next interval. This cadence balances measurement accuracy with CPU headroom.
inline constexpr uint32_t SAMPLE_INTERVAL_MS = 500;

// --- RMS Burst Sampling Configuration ---
// Nepal grid frequency is 50 Hz → one AC cycle = 20ms.
// We sample over RMS_CYCLE_COUNT full cycles to compute true RMS, which
// correctly handles the sinusoidal waveform and suppresses startup inrush
// surges that typically last 1-5 AC cycles.
inline constexpr uint8_t  RMS_CYCLE_COUNT       = 25;       // 25 cycles × 20ms = 500ms window
inline constexpr float    AC_CYCLE_PERIOD_MS    = 20.0f;    // 50 Hz → 20ms per cycle
inline constexpr uint16_t RMS_WINDOW_DURATION_MS = static_cast<uint16_t>(RMS_CYCLE_COUNT * AC_CYCLE_PERIOD_MS); // 500ms
inline constexpr uint16_t RMS_SAMPLES_PER_CYCLE = 40;       // 40 samples/cycle → 2kHz effective rate
inline constexpr uint16_t RMS_TOTAL_SAMPLES     = RMS_CYCLE_COUNT * RMS_SAMPLES_PER_CYCLE; // 1000 samples
inline constexpr uint16_t RMS_SAMPLE_INTERVAL_US = static_cast<uint16_t>(
    (AC_CYCLE_PERIOD_MS * 1000.0f) / RMS_SAMPLES_PER_CYCLE); // 500µs between samples

// Number of RMS readings in the rolling average smoothing window.
// 6 readings × 500ms interval = 3000ms window, which further suppresses
// motor inrush transients lasting 1-2 seconds beyond what RMS alone handles.
inline constexpr uint8_t SMOOTHING_WINDOW_SIZE = 6;

// --- WiFi Configuration ---
// Maximum number of connection retry attempts before giving up.
inline constexpr int WIFI_MAX_RETRY = 10;

// Timeout (ms) to wait for IP address acquisition after calling esp_wifi_start().
inline constexpr uint32_t WIFI_CONNECT_TIMEOUT_MS = 30000;

// --- Network Configuration ---
// 10-second interval balances telemetry freshness against bandwidth
// constraints typical of Nepal's residential internet connections.
inline constexpr uint32_t NETWORK_POLL_INTERVAL_MS = 10000;

// --- Relay GPIO Configuration ---
// Output-capable pins that avoid boot-strapping and UART0 conflicts.
// Active-high: GPIO HIGH energizes the relay via optocoupler.
inline constexpr gpio_num_t RELAY_PIN_1 = GPIO_NUM_25;  // Water heater (highest shed priority)
inline constexpr gpio_num_t RELAY_PIN_2 = GPIO_NUM_26;  // Space heater
inline constexpr gpio_num_t RELAY_PIN_3 = GPIO_NUM_27;  // EV charger / pump motor

inline constexpr int RELAY_COUNT = 3;

// Staggered restoration delay (ms) between relay reconnections to prevent
// simultaneous inrush current from motor-driven appliances.
inline constexpr uint32_t RESTORE_STAGGER_MS = 500;

// --- Task Configuration ---
inline constexpr uint32_t TASK_STACK_SIZE = 4096;
