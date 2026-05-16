/**
 * @file network_task.c
 * @brief Network synchronization task implementation for backend communication.
 *
 * Implements Task_NetworkSync which runs on Core 0 in a 10-second polling loop.
 * Each iteration reads the current smoothed power average from shared state,
 * constructs a JSON telemetry payload, POSTs it to the backend server using
 * esp_http_client, and processes the response for peak-stress instructions.
 *
 * Core 0 isolation ensures that HTTP latency (DNS resolution, TLS handshake,
 * server response time) cannot preempt or delay the sensor sampling task on
 * Core 1. The 10-second interval balances telemetry freshness against bandwidth
 * constraints typical of Nepal's residential internet connections.
 */

#include "features/network/network_task.h"
#include "core/config.h"
#include "core/shared_state.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_http_client.h"
#include "esp_log.h"

#include <stdio.h>
#include <string.h>

/* Log tag for network task messages — used by ESP_LOGW/ESP_LOGI macros
 * to identify the source module in serial output. */
static const char *TAG = "network_task";

/* Backend telemetry endpoint URL. In production this would be configured
 * via NVS or Kconfig, but a compile-time default enables initial development
 * and testing without runtime configuration infrastructure. */
#define BACKEND_TELEMETRY_URL  "http://192.168.1.100:8080/telemetry"

/* Maximum length of the JSON payload buffer. The telemetry payload is small
 * (gateway_id + power_watts), so 128 bytes provides ample headroom. */
#define PAYLOAD_BUFFER_SIZE    128

/* HTTP response buffer size. The backend response contains an acknowledged
 * flag and peak_stress_active boolean — 256 bytes is sufficient. */
#define RESPONSE_BUFFER_SIZE   256

/**
 * @brief Transmit telemetry data to the backend and process the response.
 *
 * Constructs a JSON payload with the current power reading, sends it via
 * HTTP POST, and parses the response for peak-stress instructions. Returns
 * true on successful communication, false on any failure.
 *
 * @param power_watts Current smoothed power average to report.
 * @return true if POST succeeded and response was received, false otherwise.
 */
static bool network_post_telemetry(float power_watts)
{
    char payload[PAYLOAD_BUFFER_SIZE];
    char response_buffer[RESPONSE_BUFFER_SIZE];
    int content_length = 0;

    /* Format the telemetry JSON payload. Using snprintf for safety against
     * buffer overflow — the format string produces ~60 bytes maximum. */
    snprintf(payload, sizeof(payload),
             "{\"gateway_id\":\"esp32-gw-001\",\"power_watts\":%.2f}",
             power_watts);

    /* Configure the HTTP client for a POST request to the telemetry endpoint.
     * Transport type is set explicitly to TCP (no TLS) for initial development;
     * production deployments should upgrade to HTTPS with certificate pinning. */
    esp_http_client_config_t config = {
        .url = BACKEND_TELEMETRY_URL,
        .method = HTTP_METHOD_POST,
        .timeout_ms = 5000,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL) {
        ESP_LOGW(TAG, "Failed to initialize HTTP client");
        return false;
    }

    /* Set content type and attach the JSON payload body. */
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, payload, strlen(payload));

    /* Execute the HTTP request. This blocks until response is received
     * or the 5-second timeout expires. */
    esp_err_t err = esp_http_client_perform(client);

    if (err != ESP_OK) {
        ESP_LOGW(TAG, "HTTP POST failed: %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
        return false;
    }

    int status_code = esp_http_client_get_status_code(client);
    content_length = esp_http_client_get_content_length(client);

    if (status_code == 200) {
        ESP_LOGI(TAG, "Telemetry POST successful, response length=%d", content_length);

        /* Read response body for peak-stress instructions.
         * The backend returns JSON with a peak_stress_active flag that
         * indicates whether the grid is under peak load conditions. */
        if (content_length > 0 && content_length < RESPONSE_BUFFER_SIZE) {
            int read_len = esp_http_client_read_response(client, response_buffer,
                                                         sizeof(response_buffer) - 1);
            if (read_len > 0) {
                response_buffer[read_len] = '\0';

                /* Parse peak-stress instruction from response.
                 * A simple string search suffices for the boolean flag;
                 * a full JSON parser would add unnecessary code size for
                 * this single-field extraction. */
                if (strstr(response_buffer, "\"peak_stress_active\":true") != NULL) {
                    ESP_LOGI(TAG, "Peak stress ACTIVE — shedding may be required");
                } else {
                    ESP_LOGI(TAG, "Peak stress inactive — normal operation");
                }
            }
        }
    } else {
        ESP_LOGW(TAG, "Backend returned HTTP %d", status_code);
    }

    esp_http_client_cleanup(client);
    return (status_code == 200);
}

void Task_NetworkSync(void *pvParameters)
{
    gateway_state_t *state = (gateway_state_t *)pvParameters;

    ESP_LOGI(TAG, "Network sync task started, poll interval=%dms",
             NETWORK_POLL_INTERVAL_MS);

    /* Infinite polling loop — the task never exits under normal operation.
     * On communication failure, the loop continues to the next interval
     * rather than halting, ensuring resilience against transient network issues. */
    for (;;) {
        /* Read current smoothed average from shared state. The mutex is held
         * only for the duration of the read (~microseconds), minimizing
         * contention with the sensor task's write operations. */
        float current_power = shared_state_read_average(state);

        ESP_LOGI(TAG, "Current power average: %.2f W", current_power);

        /* Transmit telemetry to backend. On failure, log warning and continue —
         * the next iteration will retry with fresh data. */
        if (!network_post_telemetry(current_power)) {
            ESP_LOGW(TAG, "Telemetry transmission failed, will retry next interval");
        }

        /* Delay for the configured polling interval before next transmission.
         * vTaskDelay yields the CPU to lower-priority tasks during the wait,
         * which is appropriate since network sync is not time-critical. */
        vTaskDelay(pdMS_TO_TICKS(NETWORK_POLL_INTERVAL_MS));
    }
}
