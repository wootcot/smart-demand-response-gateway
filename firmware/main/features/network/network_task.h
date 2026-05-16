/**
 * @file network_task.h
 * @brief Public API for the network synchronization feature module.
 *
 * Declares the FreeRTOS task function responsible for periodic backend
 * communication. The network task runs on Core 0, isolated from the
 * sensor sampling task on Core 1, to prevent HTTP latency from disrupting
 * ADC timing. Communication uses esp_http_client for telemetry POST
 * requests and peak-stress instruction retrieval.
 */

#ifndef FEATURES_NETWORK_NETWORK_TASK_H
#define FEATURES_NETWORK_NETWORK_TASK_H

#include "core/shared_state.h"

/**
 * @brief FreeRTOS task function for network synchronization.
 *
 * Implements a 10-second polling loop that reads the current smoothed
 * power average from shared state (mutex-protected), transmits it to
 * the backend via HTTP POST, and processes the response for peak-stress
 * instructions. On communication failure, logs a warning and retries
 * on the next interval without halting.
 *
 * @param pvParameters Pointer to the shared gateway_state_t structure.
 *                     Must remain valid for the lifetime of the task.
 */
void Task_NetworkSync(void *pvParameters);

#endif /* FEATURES_NETWORK_NETWORK_TASK_H */
