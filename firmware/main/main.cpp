/**
 * @file main.cpp
 * @brief Firmware entry point for the Nepal Grid Peak Load Controller gateway.
 *
 * Orchestrates the dual-core FreeRTOS task architecture using statically
 * allocated class instances:
 *   - Sensor Task on Core 1 (priority 2): deterministic ADC sampling
 *   - Network Task on Core 0 (priority 1): backend communication
 *
 * Core separation ensures network I/O latency (WiFi interrupts, TCP retransmits)
 * cannot introduce jitter into the 100ms ADC sampling loop.
 *
 * All objects are statically allocated at compile time to protect the heap.
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "core/config.h"
#include "core/gateway_state.hpp"
#include "features/sensor/current_sensor.hpp"
#include "features/relay/relay_controller.hpp"
#include "features/network/network_client.hpp"

static const char *TAG = "main";

// Statically allocate all instances at compile time to protect the heap
static GatewayState gateway_state;
static CurrentSensor ct_sensor(ADC_CHAN, CT_CALIBRATION_FACTOR);
static RelayController relay_controller;
static NetworkClient network_client(gateway_state, relay_controller);

void Task_ReadSensors(void *pvParameters)
{
    ESP_LOGI(TAG, "Sensor task started: RMS over %d AC cycles (%dms), "
             "%d-reading smoothing window, %ums interval",
             RMS_CYCLE_COUNT, RMS_WINDOW_DURATION_MS,
             SMOOTHING_WINDOW_SIZE, SAMPLE_INTERVAL_MS);

    while (true) {
        float current_amps = ct_sensor.read_smoothed_amps();
        gateway_state.write_power(current_amps);
        vTaskDelay(pdMS_TO_TICKS(SAMPLE_INTERVAL_MS));
    }
}

void Task_NetworkSync(void *pvParameters)
{
    ESP_LOGI(TAG, "Network task started, telemetry interval=%ums", NETWORK_POLL_INTERVAL_MS);

    while (true) {
        network_client.send_telemetry();
        vTaskDelay(pdMS_TO_TICKS(NETWORK_POLL_INTERVAL_MS));
    }
}

extern "C" void app_main(void)
{
    // NVS is required by the Wi-Fi driver for RF calibration storage
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    if (!gateway_state.init()) {
        ESP_LOGE(TAG, "Failed to initialize gateway state");
        return;
    }

    if (!ct_sensor.init()) {
        ESP_LOGE(TAG, "Failed to initialize current sensor");
        return;
    }

    if (!relay_controller.init()) {
        ESP_LOGE(TAG, "Failed to initialize relay controller");
        return;
    }

    if (!network_client.init()) {
        ESP_LOGE(TAG, "Failed to initialize network client");
        return;
    }

    // Sensor Task pinned to Core 1 (priority 2)
    // Core 1 isolation provides deterministic ADC timing free from
    // network interrupt interference on Core 0.
    xTaskCreatePinnedToCore(Task_ReadSensors, "SensorTask",
                            TASK_STACK_SIZE, nullptr, 2, nullptr, 1);

    // Network Task pinned to Core 0 (priority 1)
    // Lower priority ensures sampling is never preempted by network operations.
    xTaskCreatePinnedToCore(Task_NetworkSync, "NetworkTask",
                            TASK_STACK_SIZE, nullptr, 1, nullptr, 0);

    ESP_LOGI(TAG, "Gateway started: Sensor(Core1) + Network(Core0)");
}
