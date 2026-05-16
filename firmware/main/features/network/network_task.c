/**
 * @file network_task.c
 * @brief Network synchronization task using full-duplex WebSocket communication.
 *
 * Implements Task_NetworkSync which runs on Core 0 maintaining a persistent
 * WebSocket connection to the backend control server. The connection enables:
 *   - Low-latency telemetry streaming (gateway → backend)
 *   - Instant peak-stress command reception (backend → gateway)
 *
 * Unlike HTTP polling, the WebSocket connection remains open, allowing the
 * backend to push load-shedding commands to the gateway with sub-second
 * latency. Telemetry is still sent at configurable intervals, but peak-stress
 * instructions arrive immediately without waiting for the next poll cycle.
 *
 * Core 0 isolation ensures that WebSocket I/O (TCP keepalives, frame parsing,
 * reconnection attempts) cannot preempt the sensor sampling task on Core 1.
 */

#include "features/network/network_task.h"
#include "features/relay/relay_control.h"
#include "core/config.h"
#include "core/shared_state.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_websocket_client.h"
#include "esp_log.h"
#include "esp_event.h"

#include <stdio.h>
#include <string.h>

/* Log tag for network task messages. */
static const char *TAG = "network_task";

/* Backend WebSocket endpoint URL. The gateway_id query parameter identifies
 * this device to the backend's connection hub for targeted command delivery. */
#define BACKEND_WS_URL  "ws://192.168.1.100:8080/ws?gateway_id=esp32-gw-001"

/* Maximum length of the JSON telemetry payload buffer. */
#define PAYLOAD_BUFFER_SIZE    128

/* Flag indicating whether the WebSocket connection is currently established.
 * Set by the event handler on CONNECTED, cleared on DISCONNECTED/ERROR. */
static volatile bool s_ws_connected = false;

/* Pointer to shared state, set during task initialization for use in the
 * event callback which cannot receive arbitrary parameters. */
static gateway_state_t *s_shared_state = NULL;

/**
 * @brief Handle peak-stress instruction received from the backend.
 *
 * Parses the incoming WebSocket frame for peak_stress_active flag and
 * triggers relay control on state transitions. This executes with minimal
 * latency since the backend pushes commands immediately over the open connection.
 *
 * @param data   Raw JSON data from the WebSocket frame.
 * @param len    Length of the data buffer.
 */
static void network_handle_ws_message(const char *data, int len)
{
    if (s_shared_state == NULL || data == NULL || len <= 0) {
        return;
    }

    /* Parse peak-stress instruction from the backend's push message.
     * The backend sends {"type":"peak_stress","peak_stress_active":true/false}
     * or {"type":"ack","acknowledged":true,"peak_stress_active":true/false}. */
    bool peak_active = (strnstr(data, "\"peak_stress_active\":true", len) != NULL);
    bool was_active = shared_state_get_peak_stress(s_shared_state);

    shared_state_set_peak_stress(s_shared_state, peak_active);

    /* Execute relay control only on state transitions to avoid redundant
     * GPIO writes on every acknowledgment frame. */
    if (peak_active && !was_active) {
        ESP_LOGW(TAG, "Peak stress ACTIVATED via WebSocket — executing load shedding");
        relay_shed_loads();
    } else if (!peak_active && was_active) {
        ESP_LOGI(TAG, "Peak stress CLEARED via WebSocket — restoring loads");
        relay_restore_loads();
    }
}

/**
 * @brief WebSocket event handler callback.
 *
 * Processes connection lifecycle events and incoming data frames from the
 * backend. Runs in the context of the esp_websocket_client task, so relay
 * control calls here execute on Core 0 alongside the network task.
 */
static void websocket_event_handler(void *handler_args, esp_event_base_t base,
                                     int32_t event_id, void *event_data)
{
    esp_websocket_event_data_t *ws_event = (esp_websocket_event_data_t *)event_data;

    switch (event_id) {
        case WEBSOCKET_EVENT_CONNECTED:
            ESP_LOGI(TAG, "WebSocket connected to backend");
            s_ws_connected = true;
            break;

        case WEBSOCKET_EVENT_DISCONNECTED:
            ESP_LOGW(TAG, "WebSocket disconnected — will auto-reconnect");
            s_ws_connected = false;
            break;

        case WEBSOCKET_EVENT_DATA:
            /* Process incoming frames from the backend (peak-stress commands, acks). */
            if (ws_event->op_code == 0x01 && ws_event->data_len > 0) {
                /* Text frame received — parse for peak-stress instructions. */
                network_handle_ws_message(ws_event->data_ptr, ws_event->data_len);
            }
            break;

        case WEBSOCKET_EVENT_ERROR:
            ESP_LOGW(TAG, "WebSocket error occurred");
            s_ws_connected = false;
            break;

        default:
            break;
    }
}

void Task_NetworkSync(void *pvParameters)
{
    s_shared_state = (gateway_state_t *)pvParameters;

    ESP_LOGI(TAG, "Network sync task started (WebSocket mode), telemetry interval=%dms",
             NETWORK_POLL_INTERVAL_MS);

    /* Configure the WebSocket client with automatic reconnection.
     * The esp_websocket_client handles TCP connection management, frame parsing,
     * ping/pong keepalives, and reconnection attempts internally. */
    esp_websocket_client_config_t ws_cfg = {
        .uri = BACKEND_WS_URL,
        .reconnect_timeout_ms = 5000,
        .network_timeout_ms = 10000,
    };

    esp_websocket_client_handle_t client = esp_websocket_client_init(&ws_cfg);
    if (client == NULL) {
        ESP_LOGE(TAG, "Failed to initialize WebSocket client");
        /* Fall into idle loop to avoid watchdog trigger. */
        for (;;) {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }

    /* Register event handler for connection lifecycle and incoming data. */
    esp_websocket_register_events(client, WEBSOCKET_EVENT_ANY,
                                   websocket_event_handler, NULL);

    /* Start the WebSocket client — this initiates the TCP connection and
     * WebSocket handshake in the background. The client automatically
     * reconnects on disconnection with exponential backoff. */
    esp_err_t err = esp_websocket_client_start(client);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start WebSocket client: %s", esp_err_to_name(err));
        esp_websocket_client_destroy(client);
        for (;;) {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }

    /* Telemetry transmission loop. Even though the WebSocket is full-duplex,
     * we still send telemetry at regular intervals rather than on every ADC
     * sample to avoid flooding the backend with 10 Hz data. The key advantage
     * is that peak-stress commands arrive instantly via the event handler
     * without waiting for this loop's next iteration. */
    char payload[PAYLOAD_BUFFER_SIZE];

    for (;;) {
        if (s_ws_connected) {
            float current_power = shared_state_read_average(s_shared_state);

            /* Format telemetry as JSON and send over the WebSocket connection.
             * The backend acknowledges each frame with the current peak-stress state. */
            int len = snprintf(payload, sizeof(payload),
                "{\"type\":\"telemetry\",\"gateway_id\":\"esp32-gw-001\",\"power_watts\":%.2f}",
                current_power);

            if (esp_websocket_client_send_text(client, payload, len, pdMS_TO_TICKS(1000)) < 0) {
                ESP_LOGW(TAG, "Failed to send telemetry frame");
            } else {
                ESP_LOGI(TAG, "Telemetry sent: %.2f W", current_power);
            }
        } else {
            ESP_LOGW(TAG, "WebSocket not connected, skipping telemetry");
        }

        /* Delay between telemetry transmissions. Peak-stress commands still
         * arrive instantly via the event handler regardless of this interval. */
        vTaskDelay(pdMS_TO_TICKS(NETWORK_POLL_INTERVAL_MS));
    }
}
