/**
 * @file network_task.h
 * @brief Public API for the network synchronization feature module.
 *
 * Declares the FreeRTOS task function responsible for maintaining a persistent
 * full-duplex WebSocket connection to the backend control server. The network
 * task runs on Core 0, isolated from the sensor sampling task on Core 1, to
 * prevent WebSocket I/O from disrupting ADC timing.
 *
 * The WebSocket connection enables:
 *   - Streaming telemetry from gateway to backend at configurable intervals
 *   - Instant peak-stress command reception from backend (sub-second latency)
 *   - Automatic reconnection on network failures
 */

#ifndef FEATURES_NETWORK_NETWORK_TASK_H
#define FEATURES_NETWORK_NETWORK_TASK_H

#include "core/shared_state.h"

/**
 * @brief FreeRTOS task function for WebSocket-based network synchronization.
 *
 * Establishes and maintains a persistent WebSocket connection to the backend.
 * Sends telemetry frames at NETWORK_POLL_INTERVAL_MS intervals and receives
 * peak-stress commands via the event-driven callback. On disconnection, the
 * esp_websocket_client automatically reconnects with exponential backoff.
 *
 * @param pvParameters Pointer to the shared gateway_state_t structure.
 *                     Must remain valid for the lifetime of the task.
 */
void Task_NetworkSync(void *pvParameters);

#endif /* FEATURES_NETWORK_NETWORK_TASK_H */
