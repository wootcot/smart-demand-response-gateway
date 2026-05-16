/**
 * @file main.c
 * @brief Firmware entry point for the Nepal Grid Peak Load Controller gateway.
 *
 * Implements app_main(), the ESP-IDF application entry point that initializes
 * shared state and spawns the dual-core FreeRTOS task architecture:
 *   - Sensor Task on Core 1 (priority 2): deterministic ADC sampling
 *   - Network Task on Core 0 (priority 1): backend communication
 *
 * Core separation ensures network I/O latency (WiFi interrupts, TCP retransmits)
 * cannot introduce jitter into the 100ms ADC sampling loop. After task creation,
 * app_main() does not return — the FreeRTOS scheduler takes over execution.
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "core/shared_state.h"
#include "features/sensor/sensor_task.h"
#include "features/network/network_task.h"
#include "features/relay/relay_control.h"
#include "features/relay/relay_control.h"

static const char *TAG = "main";

/** Stack size for both sensor and network tasks (4096 bytes). */
#define TASK_STACK_SIZE 4096

/** Shared gateway state instance, lives for the entire application lifetime. */
static gateway_state_t g_gateway_state;

void app_main(void)
{
    /* Initialize shared state and mutex before creating any tasks. */
    esp_err_t err = shared_state_init(&g_gateway_state);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize shared state: %s", esp_err_to_name(err));
        return;
    }

    /* Initialize relay control GPIOs before network task starts issuing commands.
     * Relays start de-energized (loads connected) as a fail-safe default. */
    err = relay_control_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize relay control: %s", esp_err_to_name(err));
        return;
    }

    /* Initialize relay control GPIOs for the opto-isolated relay array.
     * Must complete before the network task starts issuing shed/restore commands. */
    err = relay_control_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize relay control: %s", esp_err_to_name(err));
        return;
    }

    /* Create Sensor Task pinned to Core 1 (priority 2).
     * Core 1 isolation provides deterministic ADC timing free from
     * network interrupt interference on Core 0. */
    BaseType_t ret = xTaskCreatePinnedToCore(
        Task_ReadSensors,
        "SensorTask",
        TASK_STACK_SIZE,
        &g_gateway_state,
        2,
        NULL,
        1
    );
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create Sensor Task");
        return;
    }

    /* Create Network Task pinned to Core 0 (priority 1).
     * Lower priority than sensor task ensures sampling is never preempted
     * by network operations on the same core during scheduler decisions. */
    ret = xTaskCreatePinnedToCore(
        Task_NetworkSync,
        "NetworkTask",
        TASK_STACK_SIZE,
        &g_gateway_state,
        1,
        NULL,
        0
    );
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create Network Task");
        return;
    }

    ESP_LOGI(TAG, "Gateway started: Sensor(Core1) + Network(Core0)");

    /* app_main() does not return. The FreeRTOS scheduler now owns execution.
     * If app_main were to return, the idle task would reclaim this stack. */
}
