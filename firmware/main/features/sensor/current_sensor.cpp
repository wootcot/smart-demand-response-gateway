/**
 * @file current_sensor.cpp
 * @brief CT clamp ADC sampling with true RMS measurement over multiple AC cycles.
 *
 * Uses the modern esp_adc oneshot API (ESP-IDF v5.x) for burst sampling.
 * Each measurement cycle rapidly samples the CT clamp at ~2kHz over 25 full
 * AC cycles (500ms at 50Hz), then computes the root-mean-square value.
 *
 * Two-stage smoothing strategy:
 *   1. RMS over 25 AC cycles — correctly handles sinusoidal waveform and
 *      inherently rejects short startup inrush surges (1-5 cycles) by
 *      averaging their energy contribution across the full measurement window.
 *   2. Rolling average of 6 consecutive RMS readings (3s total) — further
 *      suppresses motor inrush transients lasting 1-2 seconds.
 *
 * This ensures the gateway does not misinterpret a safe startup surge as
 * an emergency grid stress overload event.
 */

#include "features/sensor/current_sensor.hpp"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <cmath>

static const char *TAG = "current_sensor";

CurrentSensor::CurrentSensor(adc_channel_t channel, float cal_factor) noexcept
    : channel(channel), calibration_factor(cal_factor)
{
}

bool CurrentSensor::init() noexcept
{
    // Configure ADC unit for oneshot mode (no ULP co-processor involvement)
    adc_oneshot_unit_init_cfg_t unit_cfg = {
        .unit_id = ADC_UNIT_ID,
        .clk_src = ADC_RTC_CLK_SRC_DEFAULT,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };

    esp_err_t ret = adc_oneshot_new_unit(&unit_cfg, &adc_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize ADC unit: %s", esp_err_to_name(ret));
        return false;
    }

    // Configure channel with 12-bit resolution and DB_12 attenuation.
    // DB_12 provides ~0-3.1V input range which accommodates the SCT-013
    // CT clamp output after the burden resistor voltage divider.
    adc_oneshot_chan_cfg_t chan_cfg = {
        .atten = ADC_ATTEN_LVL,
        .bitwidth = ADC_BW,
    };

    ret = adc_oneshot_config_channel(adc_handle, channel, &chan_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure ADC channel: %s", esp_err_to_name(ret));
        adc_oneshot_del_unit(adc_handle);
        adc_handle = nullptr;
        return false;
    }

    ESP_LOGI(TAG, "ADC initialized: channel=%d, cal_factor=%.2f, "
             "RMS window=%d cycles (%dms), smoothing=%d readings",
             channel, calibration_factor,
             RMS_CYCLE_COUNT, RMS_WINDOW_DURATION_MS, SMOOTHING_WINDOW_SIZE);
    return true;
}

float CurrentSensor::compute_rms_amps() noexcept
{
    // Burst-sample the CT clamp over RMS_CYCLE_COUNT AC cycles.
    // At 50Hz with 40 samples/cycle, we take 1000 samples at 500µs intervals
    // spanning exactly 25 full AC cycles (500ms).
    //
    // The RMS calculation squares each sample, accumulates the sum, then takes
    // the square root of the mean. This correctly handles the sinusoidal AC
    // waveform — a short inrush spike lasting 1-5 cycles contributes only a
    // fraction of its energy to the 25-cycle RMS window, preventing false
    // overload detection.

    float sum_of_squares = 0.0f;
    uint16_t valid_samples = 0;

    for (uint16_t i = 0; i < RMS_TOTAL_SAMPLES; i++) {
        int raw_value = 0;
        esp_err_t ret = adc_oneshot_read(adc_handle, channel, &raw_value);

        if (ret == ESP_OK) {
            // Convert raw ADC count to amperes via calibration factor
            float amps = static_cast<float>(raw_value) * calibration_factor;
            sum_of_squares += amps * amps;
            valid_samples++;
        }
        // Failed reads are silently skipped — we only need a majority of
        // samples for an accurate RMS estimate.

        // Busy-wait for the inter-sample interval (~500µs).
        // esp_rom_delay_us provides microsecond-precision delay without
        // yielding to the scheduler, maintaining deterministic sample timing.
        if (i < RMS_TOTAL_SAMPLES - 1) {
            esp_rom_delay_us(RMS_SAMPLE_INTERVAL_US);
        }
    }

    // Require at least 75% of samples to be valid for a trustworthy RMS reading
    uint16_t min_valid = (RMS_TOTAL_SAMPLES * 3) / 4;
    if (valid_samples < min_valid) {
        ESP_LOGW(TAG, "RMS burst: only %u/%u valid samples (need %u), discarding",
                 valid_samples, RMS_TOTAL_SAMPLES, min_valid);
        return -1.0f;
    }

    float mean_square = sum_of_squares / static_cast<float>(valid_samples);
    float rms_amps = sqrtf(mean_square);

    return rms_amps;
}

float CurrentSensor::read_smoothed_amps() noexcept
{
    float rms_amps = compute_rms_amps();

    if (rms_amps < 0.0f) {
        // Burst measurement failed — return last known rolling average
        // without modifying the smoothing buffer.
        ESP_LOGW(TAG, "RMS measurement failed, returning cached average");
        float sum = 0.0f;
        for (uint8_t i = 0; i < SMOOTHING_WINDOW_SIZE; i++) {
            sum += smoothing_window[i];
        }
        return sum / static_cast<float>(SMOOTHING_WINDOW_SIZE);
    }

    // Write RMS reading into circular buffer at current index
    smoothing_window[window_index] = rms_amps;
    window_index = (window_index + 1) % SMOOTHING_WINDOW_SIZE;

    // Compute arithmetic mean over the smoothing window
    float sum = 0.0f;
    for (uint8_t i = 0; i < SMOOTHING_WINDOW_SIZE; i++) {
        sum += smoothing_window[i];
    }

    float smoothed = sum / static_cast<float>(SMOOTHING_WINDOW_SIZE);

    ESP_LOGD(TAG, "RMS=%.2fA, smoothed=%.2fA", rms_amps, smoothed);
    return smoothed;
}
