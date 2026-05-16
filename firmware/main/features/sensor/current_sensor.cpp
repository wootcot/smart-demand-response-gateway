/**
 * @file current_sensor.cpp
 * @brief CT clamp ADC sampling and rolling average smoothing implementation.
 *
 * Uses the modern esp_adc oneshot API (ESP-IDF v5.x) for single-shot readings.
 * Oneshot mode is appropriate because we need a single reading per 100ms cycle,
 * not continuous DMA streaming.
 *
 * Failed ADC reads are logged and skipped — the smoothing buffer is not modified,
 * preserving the integrity of the rolling average. This design ensures transient
 * hardware glitches do not corrupt the power measurement baseline.
 */

#include "features/sensor/current_sensor.hpp"
#include "esp_log.h"

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
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };

    esp_err_t ret = adc_oneshot_new_unit(&unit_cfg, &adc_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize ADC unit: %s", esp_err_to_name(ret));
        return false;
    }

    // Configure channel with 12-bit resolution and DB_11 attenuation.
    // DB_11 provides ~0-3.1V input range which accommodates the SCT-013
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

    ESP_LOGI(TAG, "ADC initialized: channel=%d, cal_factor=%.2f", channel, calibration_factor);
    return true;
}

float CurrentSensor::read_smoothed_amps() noexcept
{
    int raw_value = 0;
    esp_err_t ret = adc_oneshot_read(adc_handle, channel, &raw_value);

    if (ret != ESP_OK) {
        // Failed reads are skipped — smoothing buffer retains previous contents,
        // preserving rolling average integrity across transient hardware glitches.
        ESP_LOGW(TAG, "ADC read failed: %s, skipping sample", esp_err_to_name(ret));
        // Return last known average rather than corrupting with a zero
        float sum = 0.0f;
        for (uint8_t i = 0; i < SMOOTHING_WINDOW_SIZE; i++) {
            sum += smoothing_window[i];
        }
        return sum / static_cast<float>(SMOOTHING_WINDOW_SIZE);
    }

    // Apply calibration factor to convert raw ADC count to amperes
    float amps = static_cast<float>(raw_value) * calibration_factor;

    // Write into circular buffer at current index, overwriting oldest sample
    smoothing_window[window_index] = amps;
    window_index = (window_index + 1) % SMOOTHING_WINDOW_SIZE;

    // Compute arithmetic mean over the entire window
    float sum = 0.0f;
    for (uint8_t i = 0; i < SMOOTHING_WINDOW_SIZE; i++) {
        sum += smoothing_window[i];
    }

    return sum / static_cast<float>(SMOOTHING_WINDOW_SIZE);
}
