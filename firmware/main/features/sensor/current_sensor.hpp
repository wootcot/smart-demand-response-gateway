/**
 * @file current_sensor.hpp
 * @brief CT clamp current sensor using ADS1115 16-bit ADC over I2C.
 *
 * Encapsulates the ESP-IDF I2C master driver, ADS1115 register configuration,
 * RMS burst sampling, and a fixed-size smoothing window into a single testable
 * class. No heap allocations — all buffers are compile-time fixed arrays.
 *
 * Hardware connection (per interconnection guide):
 *   ESP32 GPIO22 (SCL) ──── ADS1115 SCL
 *   ESP32 GPIO21 (SDA) ──── ADS1115 SDA
 *   ADS1115 VDD ──── 3.3V
 *   ADS1115 ADDR ──── GND (I2C address 0x48)
 *   ADS1115 A0 ──── CT sensor signal (DC-biased at 1.65V)
 *
 * Measurement strategy (two-stage smoothing):
 *   Stage 1 — True RMS over 25 AC cycles (500ms at 50Hz):
 *     Reads the ADS1115 in continuous mode at 860 SPS (~430 samples),
 *     subtracts the DC bias midpoint, then computes the root-mean-square
 *     value. This correctly handles the sinusoidal AC waveform and
 *     inherently rejects short startup inrush surges (which typically
 *     last 1-5 AC cycles) by averaging their energy contribution across
 *     the full 25-cycle window.
 *
 *   Stage 2 — Rolling average of RMS readings:
 *     A 6-sample circular buffer (6 × 500ms = 3s) further suppresses
 *     motor inrush transients lasting 1-2 seconds, ensuring the gateway
 *     does not misinterpret a safe startup surge as an emergency grid
 *     stress overload event.
 */

#pragma once

#include <cstdint>
#include "driver/i2c_master.h"
#include "core/config.h"

class CurrentSensor {
private:
    i2c_master_bus_handle_t bus_handle = nullptr;
    i2c_master_dev_handle_t dev_handle = nullptr;
    float calibration_factor;

    // Fixed-size circular buffer for rolling average of RMS readings (no heap)
    float smoothing_window[SMOOTHING_WINDOW_SIZE] = {0};
    uint8_t window_index = 0;

    // Write a 16-bit value to an ADS1115 register.
    bool write_register(uint8_t reg, uint16_t value) noexcept;

    // Read a 16-bit value from an ADS1115 register.
    bool read_register(uint8_t reg, int16_t &value) noexcept;

    // Compute true RMS current over RMS_CYCLE_COUNT AC cycles via burst sampling.
    // Returns RMS amperes, or -1.0f if the burst encountered too many I2C failures.
    float compute_rms_amps() noexcept;

public:
    // Explicit constructor ensuring hardware calibration parameters are injected
    explicit CurrentSensor(float cal_factor) noexcept;
    ~CurrentSensor();

    // Non-copyable (owns hardware resource)
    CurrentSensor(const CurrentSensor&) = delete;
    CurrentSensor& operator=(const CurrentSensor&) = delete;

    // Initialize I2C bus and configure ADS1115 for continuous conversion.
    // Must be called before read_smoothed_amps().
    bool init() noexcept;

    // Perform RMS burst measurement, update smoothing window, return averaged RMS amps.
    // Returns last known average on measurement failure (smoothing buffer unchanged).
    float read_smoothed_amps() noexcept;
};
