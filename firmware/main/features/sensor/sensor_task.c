/**
 * @file sensor_task.c
 * @brief FreeRTOS task implementation for CT clamp ADC sampling and smoothing.
 *
 * Implements Task_ReadSensors which runs on Core 1 at priority 2. The task
 * configures the ESP32's ADC oneshot driver for single-shot readings from the
 * CT clamp signal on GPIO34 (ADC1_CHANNEL_6), then continuously samples at
 * 100ms intervals. Each successful reading is fed into the rolling average
 * smoothing window, and the resulting average is written to the mutex-protected
 * shared state for the network task to transmit.
 *
 * Failed ADC reads are logged and skipped — the smoothing buffer is not modified,
 * preserving the integrity of the rolling average. This design ensures transient
 * hardware glitches do not corrupt the power measurement baseline.
 */

#include "features/sensor/sensor_task.h"
#include "features/sensor/smoothing.h"
#include "core/config.h"
#include "core/shared_state.h"

#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "sensor_task";

void Task_ReadSensors(void *pvParameters)
{
    gateway_state_t *state = (gateway_state_t *)pvParameters;

    /* --- ADC Oneshot Driver Configuration ---
     * Using the modern esp_adc oneshot API (ESP-IDF v5.x) rather than the
     * deprecated adc1_get_raw() legacy API. Oneshot mode is appropriate here
     * because we need a single reading per 100ms cycle, not continuous DMA streaming. */
    adc_oneshot_unit_handle_t adc_handle = NULL;

    adc_oneshot_unit_init_cfg_t unit_cfg = {
        .unit_id = ADC_UNIT,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };

    esp_err_t ret = adc_oneshot_new_unit(&unit_cfg, &adc_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize ADC unit: %s", esp_err_to_name(ret));
        /* Cannot proceed without ADC — task enters idle loop to avoid watchdog
         * trigger while allowing system to continue operating on network task. */
        for (;;) {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }

    /* Configure the specific ADC channel with 12-bit resolution and DB_11
     * attenuation. DB_11 provides ~0-3.1V input range which accommodates the
     * SCT-013 CT clamp output after the burden resistor voltage divider. */
    adc_oneshot_chan_cfg_t chan_cfg = {
        .atten = ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH,
    };

    ret = adc_oneshot_config_channel(adc_handle, ADC_CHANNEL, &chan_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure ADC channel: %s", esp_err_to_name(ret));
        adc_oneshot_del_unit(adc_handle);
        for (;;) {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }

    /* Local smoothing buffer and index — these are task-local state that only
     * this task writes to. The shared state is updated atomically via the
     * mutex-protected shared_state_write_sample() accessor. */
    float smoothing_buffer[SMOOTHING_SAMPLES] = {0};
    uint8_t buffer_index = 0;

    ESP_LOGI(TAG, "Sensor task started: %d-sample window, %dms interval",
             SMOOTHING_SAMPLES, SAMPLE_INTERVAL_MS);

    /* --- Main Sampling Loop ---
     * Runs indefinitely at SAMPLE_INTERVAL_MS (100ms) cadence. Each iteration:
     * 1. Read raw ADC value (12-bit: 0-4095)
     * 2. On success: update smoothing window, write average to shared state
     * 3. On failure: skip sample, log warning, continue next cycle */
    for (;;) {
        int raw_value = 0;
        ret = adc_oneshot_read(adc_handle, ADC_CHANNEL, &raw_value);

        if (ret == ESP_OK) {
            /* Convert raw ADC count to a float representation. In a production
             * system this would apply calibration curves and CT ratio scaling;
             * for now we pass the raw value as a float for smoothing. */
            float sample = (float)raw_value;

            /* Update the local smoothing buffer and compute new rolling average */
            float avg = smoothing_update(smoothing_buffer, &buffer_index,
                                         SMOOTHING_SAMPLES, sample);

            /* Write the smoothed average to shared state under mutex protection.
             * If the mutex times out (50ms), the write is skipped — the network
             * task will read a slightly stale value on its next 10s poll. */
            shared_state_write_sample(state, avg);
        } else {
            /* Failed ADC reads are skipped entirely — the smoothing buffer retains
             * its previous contents, preserving the rolling average integrity.
             * This handles transient hardware glitches without corrupting data. */
            ESP_LOGW(TAG, "ADC read failed: %s, skipping sample", esp_err_to_name(ret));
        }

        /* Yield CPU for SAMPLE_INTERVAL_MS before next reading. vTaskDelay
         * releases the core to lower-priority tasks and the idle task. */
        vTaskDelay(pdMS_TO_TICKS(SAMPLE_INTERVAL_MS));
    }
}
