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

#ifndef CORE_CONFIG_H
#define CORE_CONFIG_H

#include "esp_adc/adc_oneshot.h"

/* ADC hardware configuration for CT clamp (SCT-013) signal acquisition.
 * ADC_ATTEN_DB_11 provides full-scale voltage range (~0-3.1V) needed to
 * capture the CT clamp's output swing without clipping. */
#define ADC_UNIT          ADC_UNIT_1
#define ADC_CHANNEL       ADC_CHANNEL_6      /* GPIO34 - input-only, no pull interference */
#define ADC_ATTEN         ADC_ATTEN_DB_11
#define ADC_BITWIDTH      ADC_BITWIDTH_12

/* Sampling interval in milliseconds for the sensor task ADC read loop.
 * 100ms provides 10 Hz effective sampling rate — sufficient for residential
 * power monitoring while leaving CPU headroom for smoothing computation. */
#define SAMPLE_INTERVAL_MS        100

/* Number of samples in the rolling average smoothing window.
 * 30 samples × 100ms interval = 3000ms window, which suppresses inrush
 * current spikes from motor-driven appliances (typically 1-2 seconds). */
#define SMOOTHING_SAMPLES         30

/* Network polling interval in milliseconds for backend communication.
 * 10-second interval balances telemetry freshness against bandwidth
 * constraints typical of Nepal's residential internet connections. */
#define NETWORK_POLL_INTERVAL_MS  10000

#endif /* CORE_CONFIG_H */
