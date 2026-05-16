/**
 * @file network_client.cpp
 * @brief WebSocket client implementation for backend communication.
 *
 * Maintains a persistent WebSocket connection to the backend control server.
 * The connection enables:
 *   - Low-latency telemetry streaming (gateway → backend)
 *   - Instant peak-stress command reception (backend → gateway)
 *
 * Core 0 isolation ensures that WebSocket I/O (TCP keepalives, frame parsing,
 * reconnection attempts) cannot preempt the sensor sampling task on Core 1.
 */

#include "features/network/network_client.hpp"
#include "core/config.h"

#include "esp_log.h"
#include "esp_event.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <cstdio>
#include <cstring>

static const char *TAG = "network";

NetworkClient::NetworkClient(GatewayState& state, RelayController& relays) noexcept
    : state(state), relays(relays)
{
}

void NetworkClient::handle_message(const char *data, int len) noexcept
{
    if (data == nullptr || len <= 0) {
        return;
    }

    // Parse peak-stress instruction from the backend's push message.
    // Backend sends {"type":"peak_stress","peak_stress_active":true/false}
    bool peak_active = (strnstr(data, "\"peak_stress_active\":true", len) != nullptr);
    bool was_active = state.get_peak_stress();

    state.set_peak_stress(peak_active);

    // Execute relay control only on state transitions to avoid redundant GPIO writes
    if (peak_active && !was_active) {
        ESP_LOGW(TAG, "Peak stress ACTIVATED — executing load shedding");
        relays.shed_loads();
    } else if (!peak_active && was_active) {
        ESP_LOGI(TAG, "Peak stress CLEARED — restoring loads");
        relays.restore_loads();
    }
}

void NetworkClient::event_handler(void *handler_args, esp_event_base_t base,
                                   int32_t event_id, void *event_data)
{
    auto *self = static_cast<NetworkClient *>(handler_args);
    auto *ws_event = static_cast<esp_websocket_event_data_t *>(event_data);

    switch (event_id) {
        case WEBSOCKET_EVENT_CONNECTED:
            ESP_LOGI(TAG, "WebSocket connected to backend");
            self->connected = true;
            break;

        case WEBSOCKET_EVENT_DISCONNECTED:
            ESP_LOGW(TAG, "WebSocket disconnected — will auto-reconnect");
            self->connected = false;
            break;

        case WEBSOCKET_EVENT_DATA:
            if (ws_event->op_code == 0x01 && ws_event->data_len > 0) {
                self->handle_message(ws_event->data_ptr, ws_event->data_len);
            }
            break;

        case WEBSOCKET_EVENT_ERROR:
            ESP_LOGW(TAG, "WebSocket error occurred");
            self->connected = false;
            break;

        default:
            break;
    }
}

bool NetworkClient::init() noexcept
{
    esp_websocket_client_config_t ws_cfg = {};
    ws_cfg.uri = WS_URL;
    ws_cfg.reconnect_timeout_ms = 5000;
    ws_cfg.network_timeout_ms = 10000;

    client = esp_websocket_client_init(&ws_cfg);
    if (client == nullptr) {
        ESP_LOGE(TAG, "Failed to initialize WebSocket client");
        return false;
    }

    // Pass `this` as handler_args so the static trampoline can dispatch to instance
    esp_websocket_register_events(client, WEBSOCKET_EVENT_ANY, event_handler, this);

    esp_err_t err = esp_websocket_client_start(client);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start WebSocket client: %s", esp_err_to_name(err));
        esp_websocket_client_destroy(client);
        client = nullptr;
        return false;
    }

    ESP_LOGI(TAG, "WebSocket client started, telemetry interval=%ums", NETWORK_POLL_INTERVAL_MS);
    return true;
}

bool NetworkClient::send_telemetry() noexcept
{
    if (!connected) {
        ESP_LOGW(TAG, "WebSocket not connected, skipping telemetry");
        return false;
    }

    float current_power = state.read_power();

    char payload[PAYLOAD_BUFFER_SIZE];
    int len = snprintf(payload, sizeof(payload),
        "{\"type\":\"telemetry\",\"gateway_id\":\"esp32-gw-001\",\"power_watts\":%.2f}",
        current_power);

    if (esp_websocket_client_send_text(client, payload, len, pdMS_TO_TICKS(1000)) < 0) {
        ESP_LOGW(TAG, "Failed to send telemetry frame");
        return false;
    }

    ESP_LOGI(TAG, "Telemetry sent: %.2f W", current_power);
    return true;
}
