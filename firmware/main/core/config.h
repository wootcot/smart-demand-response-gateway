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
#include "driver/gpio.h"

// --- ADC Configuration ---
// CT clamp (SCT-013) signal acquisition on GPIO34.
// ADC_ATTEN_DB_11 provides full-scale voltage range (~0-3.1V) needed to
// capture the CT clamp's output swing without clipping.
inline constexpr adc_unit_t     ADC_UNIT_ID   = ADC_UNIT_1;
inline constexpr adc_channel_t  ADC_CHAN       = ADC_CHANNEL_6;
inline constexpr adc_atten_t    ADC_ATTEN_LVL = ADC_ATTEN_DB_11;
inline constexpr adc_bitwidth_t ADC_BW        = ADC_BITWIDTH_12;

// TODO: Calibrate for actual SCT-013 variant and burden resistor.
// Current value (1.82) is a placeholder. Real calibration requires measuring
// known load current with a reference meter and adjusting this factor to match.
// Factor depends on: SCT-013 ratio (e.g. 30A/1V), burden resistor (e.g. 33Ω),
// voltage divider bias (mid-rail 1.65V), and ADC reference voltage.
inline constexpr float CT_CALIBRATION_FACTOR = 1.82f;

// --- Sampling Configuration ---
// 100ms provides 10 Hz effective sampling rate — sufficient for residential
// power monitoring while leaving CPU headroom for smoothing computation.
inline constexpr uint32_t SAMPLE_INTERVAL_MS = 100;

// Number of samples in the rolling average smoothing window.
// 30 samples × 100ms interval = 3000ms window, which suppresses inrush
// current spikes from motor-driven appliances (typically 1-2 seconds).
inline constexpr uint8_t SMOOTHING_WINDOW_SIZE = 30;

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
