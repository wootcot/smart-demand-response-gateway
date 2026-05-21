/**
 * @file config.h
 * @brief Compile-time constants for the Nepal Grid Peak Load Controller firmware.
 *
 * Centralizes all hardware pin definitions, sampling intervals, and system
 * timing parameters. These constants are shared across the sensor and network
 * feature modules to ensure consistent configuration without runtime overhead.
 *
 * ADC acquisition uses an external ADS1115 16-bit delta-sigma ADC connected
 * via I2C (GPIO22 SCL, GPIO21 SDA) at address 0x48 (ADDR pin tied to GND).
 * This provides significantly better resolution and linearity than the ESP32's
 * internal 12-bit SAR ADC for energy metering applications.
 */

#pragma once

#include <cstdint>
#include "driver/i2c_master.h"
#include "driver/gpio.h"

// =============================================================================
// USER CONFIGURATION — Update these values for your deployment
// =============================================================================

// Wi-Fi credentials (move to NVS or Kconfig for production)
#define WIFI_SSID "REDACTED"
#define WIFI_PASS "REDACTED"

// WebSocket backend URL (include gateway_id as query param)
#define WS_URL "ws://192.168.101.4:8080/ws?gateway_id=esp32-gw-001"

// =============================================================================
// SYSTEM CONFIGURATION — Typically no changes needed below this line
// =============================================================================

// --- I2C Configuration ---
// ADS1115 connected via I2C on default ESP32 hardware I2C pins.
inline constexpr gpio_num_t I2C_SCL_PIN       = GPIO_NUM_22;
inline constexpr gpio_num_t I2C_SDA_PIN       = GPIO_NUM_21;
inline constexpr uint32_t   I2C_FREQ_HZ       = 400000;  // 400kHz Fast Mode

// --- ADS1115 Configuration ---
// ADDR pin tied to GND → I2C address 0x48
inline constexpr uint8_t  ADS1115_ADDR        = 0x48;

// ADS1115 register addresses
inline constexpr uint8_t  ADS1115_REG_CONVERSION = 0x00;
inline constexpr uint8_t  ADS1115_REG_CONFIG     = 0x01;

// ADS1115 config register bit fields for CT sensor reading:
//   [15]    OS=1        Start single conversion
//   [14:12] MUX=100     AIN0 vs GND (single-ended, channel 0)
//   [11:9]  PGA=001     ±4.096V full-scale (allows full 0-3.3V swing)
//   [8]     MODE=0      Continuous conversion mode
//   [7:5]   DR=111      860 SPS (maximum data rate)
//   [4]     COMP_MODE=0 Traditional comparator
//   [3]     COMP_POL=0  Active low
//   [2]     COMP_LAT=0  Non-latching
//   [1:0]   COMP_QUE=11 Disable comparator
inline constexpr uint16_t ADS1115_CONFIG_CONTINUOUS =
    (0b0 << 15) |   // OS: no effect in continuous mode
    (0b100 << 12) | // MUX: AIN0 vs GND
    (0b001 << 9) |  // PGA: ±4.096V
    (0b0 << 8) |    // MODE: continuous
    (0b111 << 5) |  // DR: 860 SPS
    (0b0 << 4) |    // COMP_MODE: traditional
    (0b0 << 3) |    // COMP_POL: active low
    (0b0 << 2) |    // COMP_LAT: non-latching
    (0b11 << 0);    // COMP_QUE: disable comparator

// PGA full-scale voltage for ±4.096V gain setting.
// LSB size = 4.096V / 32768 = 0.000125V = 125µV
inline constexpr float ADS1115_LSB_VOLTAGE    = 0.000125f;

// TODO: Calibrate for actual SCT-013 variant and burden resistor.
// This factor converts the ADS1115 voltage reading to amperes.
// Derivation: SCT-013-030 outputs 1V at 30A through 33Ω burden.
// So current = voltage_across_burden / 33Ω * 30A_ratio
// Simplified: amps = (adc_voltage - midpoint) * calibration_factor
// The midpoint (1.65V) is subtracted in firmware before applying this factor.
inline constexpr float CT_CALIBRATION_FACTOR  = 30.0f;  // 30A per 1V across burden

// DC bias midpoint voltage (VCC/2 = 3.3V/2 = 1.65V)
// The 10kΩ + 10kΩ voltage divider sets this reference level.
inline constexpr float DC_BIAS_MIDPOINT_V     = 1.65f;

// --- Sampling Configuration ---
// 500ms interval between RMS measurement cycles. Each cycle reads the ADS1115
// in continuous mode at 860 SPS, collecting ~430 samples over 500ms.
inline constexpr uint32_t SAMPLE_INTERVAL_MS  = 500;

// --- RMS Burst Sampling Configuration ---
// Nepal grid frequency is 50 Hz → one AC cycle = 20ms.
// We sample over RMS_CYCLE_COUNT full cycles to compute true RMS.
inline constexpr uint8_t  RMS_CYCLE_COUNT        = 25;       // 25 cycles × 20ms = 500ms window
inline constexpr float    AC_CYCLE_PERIOD_MS     = 20.0f;    // 50 Hz → 20ms per cycle
inline constexpr uint16_t RMS_WINDOW_DURATION_MS = static_cast<uint16_t>(RMS_CYCLE_COUNT * AC_CYCLE_PERIOD_MS); // 500ms

// ADS1115 at 860 SPS → ~1.16ms per sample. Over 500ms we get ~430 samples.
// That's ~17 samples per AC cycle — sufficient for true RMS of a 50Hz sine.
inline constexpr uint16_t ADS1115_SPS            = 860;
inline constexpr uint16_t RMS_TOTAL_SAMPLES      = static_cast<uint16_t>(
    (RMS_WINDOW_DURATION_MS * ADS1115_SPS) / 1000);  // ~430 samples
inline constexpr uint16_t RMS_SAMPLE_INTERVAL_US = static_cast<uint16_t>(
    1000000 / ADS1115_SPS);  // ~1163µs between samples

// Number of RMS readings in the rolling average smoothing window.
// 6 readings × 500ms interval = 3000ms window, which further suppresses
// motor inrush transients lasting 1-2 seconds beyond what RMS alone handles.
inline constexpr uint8_t SMOOTHING_WINDOW_SIZE   = 6;

// --- WiFi Configuration ---
inline constexpr int      WIFI_MAX_RETRY         = 10;
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
