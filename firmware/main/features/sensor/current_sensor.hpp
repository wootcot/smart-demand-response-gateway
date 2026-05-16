/**
 * @file current_sensor.hpp
 * @brief CT clamp current sensor with ADC sampling and rolling average smoothing.
 *
 * Encapsulates the ESP32 ADC oneshot driver, calibration, and a fixed-size
 * smoothing window into a single testable class. No heap allocations — the
 * smoothing buffer is a compile-time fixed array.
 *
 * The smoothing window suppresses inrush current spikes from motor-driven
 * appliances (refrigerators, water pumps) which typically last 1-2 seconds.
 * A 3-second window (30 samples at 100ms intervals) ensures these transients
 * are averaged out before telemetry is transmitted to the backend.
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

    // Fixed-size circular buffer for rolling average (no heap allocations)
    float smoothing_window[SMOOTHING_WINDOW_SIZE] = {0};
    uint8_t window_index = 0;

public:
    // Explicit constructor ensuring hardware calibration parameters are injected
    explicit CurrentSensor(adc_channel_t channel, float cal_factor) noexcept;
    ~CurrentSensor() = default;

    // Non-copyable (owns hardware resource)
    CurrentSensor(const CurrentSensor&) = delete;
    CurrentSensor& operator=(const CurrentSensor&) = delete;

    // Initialize ADC hardware. Must be called before read_smoothed_amps().
    bool init() noexcept;

    // Read ADC, apply calibration, update smoothing window, return averaged amps.
    // Returns last known average on ADC read failure (smoothing buffer unchanged).
    float read_smoothed_amps() noexcept;
};
