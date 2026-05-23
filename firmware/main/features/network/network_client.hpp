/**
 * @file network_client.hpp
 * @brief Full-duplex WebSocket client for backend communication.
 *
 * Encapsulates the esp_websocket_client lifecycle: connection management,
 * telemetry transmission, and peak-stress command reception. The WebSocket
 * connection enables sub-second latency for load-shedding commands from
 * the backend while streaming telemetry at configurable intervals.
 *
 * Automatic reconnection with exponential backoff is handled internally
 * by the esp_websocket_client library.
 */

#pragma once

#include "esp_websocket_client.h"
#include "core/gateway_state.hpp"
#include "core/nvs_config.hpp"
#include "features/relay/relay_controller.hpp"

class NetworkClient {
private:
    esp_websocket_client_handle_t client = nullptr;
    volatile bool connected = false;

    // References to collaborating objects (non-owning)
    GatewayState& state;
    RelayController& relays;
    const NvsConfig& config;

    static constexpr size_t PAYLOAD_BUFFER_SIZE = 128;
    static constexpr size_t URI_BUFFER_SIZE = 192;

    void handle_message(const char *data, int len) noexcept;

    // Static trampoline for esp_event callback (C API requires function pointer)
    static void event_handler(void *handler_args, esp_event_base_t base,
                              int32_t event_id, void *event_data);

public:
    explicit NetworkClient(GatewayState& state, RelayController& relays,
                           const NvsConfig& config) noexcept;
    ~NetworkClient() = default;

    // Non-copyable (owns connection resource)
    NetworkClient(const NetworkClient&) = delete;
    NetworkClient& operator=(const NetworkClient&) = delete;

    // Initialize WebSocket client and start connection.
    bool init() noexcept;

    // Send a telemetry frame with current power reading. Returns true on success.
    bool send_telemetry() noexcept;

    // Query connection state.
    bool is_connected() const noexcept { return connected; }
};
