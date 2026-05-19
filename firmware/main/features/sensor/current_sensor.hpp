/**
 * @file current_sensor.hpp
 * @brief CT clamp current sensor with true RMS measurement and rolling average smoothing.
 *
 * Encapsulates the ESP32 ADC oneshot driver, calibration, RMS burst sampling,
 * and a fixed-size smoothing window into a single testable class. No heap
 * allocations — all buffers are compile-time fixed arrays.
 *
 * Measurement strategy (two-stage smoothing):
 *   Stage 1 — True RMS over 25 AC cycles (500ms at 50Hz):
 *     Burst-samples the CT clamp at ~2kHz (1000 samples), computes the
 *     root-mean-square value. This correctly handles the sinusoidal AC
 *     waveform and inherently rejects short startup inrush surges (which
 *     typically last 1-5 AC cycles) by averaging their energy contribution
 *     across the full 25-cycle window.
 *
 *   Stage 2 — Rolling average of RMS readings:
 *     A 6-sample circular buffer (6 × 500ms = 3s) further suppresses
 *     motor inrush transients lasting 1-2 seconds, ensuring the gateway
 *     does not misinterpret a safe startup surge as an emergency grid
 *     stress overload event.
 */

#pragma once

#include <cstdint>
#include "esp_adc/adc_oneshot.h"
#include "core/config.h"

class CurrentSensor {
private:
    adc_oneshot_unit_handle_t adc_handle = nullptr;
    adc_channel_t channel;
    float calibration_factor;

    // Fixed-size circular buffer for rolling average of RMS readings (no heap)
    float smoothing_window[SMOOTHING_WINDOW_SIZE] = {0};
    uint8_t window_index = 0;

    // Compute true RMS current over RMS_CYCLE_COUNT AC cycles via burst sampling.
    // Returns RMS amperes, or -1.0f if the burst encountered too many ADC failures.
    float compute_rms_amps() noexcept;

public:
    // Explicit constructor ensuring hardware calibration parameters are injected
    explicit CurrentSensor(adc_channel_t channel, float cal_factor) noexcept;
    ~CurrentSensor() = default;

    // Non-copyable (owns hardware resource)
    CurrentSensor(const CurrentSensor&) = delete;
    CurrentSensor& operator=(const CurrentSensor&) = delete;

    // Initialize ADC hardware. Must be called before read_smoothed_amps().
    bool init() noexcept;

    // Perform RMS burst measurement, update smoothing window, return averaged RMS amps.
    // Returns last known average on measurement failure (smoothing buffer unchanged).
    float read_smoothed_amps() noexcept;
};
