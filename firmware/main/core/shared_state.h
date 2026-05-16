/**
 * @file shared_state.h
 * @brief Thread-safe shared state interface for inter-task communication.
 *
 * Defines the gateway_state_t structure and accessor functions that enable
 * the sensor task (Core 1) and network task (Core 0) to exchange data safely.
 * All access to the shared state is mutex-protected to prevent data races
 * between the dual-core FreeRTOS tasks.
 *
 * Design rationale: Rather than using FreeRTOS queues (which would introduce
 * copy overhead and queue-full handling complexity), a mutex-protected struct
 * provides direct access semantics appropriate for a single-producer
 * (sensor task) / single-consumer (network task) pattern with infrequent reads.
 */

#ifndef CORE_SHARED_STATE_H
#define CORE_SHARED_STATE_H

#include <stdint.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_err.h"
#include "core/config.h"

/**
 * @brief Central shared state structure for the gateway.
 *
 * Holds the smoothing buffer, current write index, computed average,
 * peak stress flag from the backend, and the mutex protecting concurrent
 * access from dual-core tasks.
 */
typedef struct {
    float smoothing_buffer[SMOOTHING_SAMPLES];
    uint8_t buffer_index;
    float current_avg_watts;
    bool peak_stress_active;
    SemaphoreHandle_t mutex;
} gateway_state_t;

/**
 * @brief Initialize shared state and create the protecting mutex.
 *
 * Zeroes the smoothing buffer, resets the buffer index, and creates
 * a FreeRTOS mutex. Must be called from app_main() before any tasks
 * are created.
 *
 * @param state Pointer to the gateway state structure to initialize.
 * @return ESP_OK on success, ESP_ERR_NO_MEM if mutex creation fails.
 */
esp_err_t shared_state_init(gateway_state_t *state);

/**
 * @brief Thread-safe write: update buffer with new sample and recompute average.
 *
 * Acquires the mutex, writes the sample into the circular buffer at the
 * current index, advances the index with wraparound, recomputes the
 * rolling average, then releases the mutex.
 *
 * @param state  Pointer to the gateway state structure.
 * @param sample New power reading in watts to add to the smoothing window.
 * @return ESP_OK on success, ESP_ERR_TIMEOUT if mutex could not be acquired.
 */
esp_err_t shared_state_write_sample(gateway_state_t *state, float sample);

/**
 * @brief Thread-safe read: get current smoothed average power.
 *
 * Acquires the mutex, copies the current average value, then releases
 * the mutex. Returns 0.0 if the mutex cannot be acquired within the
 * timeout period.
 *
 * @param state Pointer to the gateway state structure.
 * @return Current smoothed average power in watts, or 0.0 on mutex timeout.
 */
float shared_state_read_average(gateway_state_t *state);

/**
 * @brief Thread-safe write: update peak stress flag from backend instruction.
 *
 * Called by the network task after receiving a peak-stress instruction
 * from the backend server. The relay control module reads this flag to
 * determine whether to shed or restore loads.
 *
 * @param state  Pointer to the gateway state structure.
 * @param active true if peak stress is active, false to clear.
 * @return ESP_OK on success, ESP_ERR_TIMEOUT if mutex could not be acquired.
 */
esp_err_t shared_state_set_peak_stress(gateway_state_t *state, bool active);

/**
 * @brief Thread-safe read: get current peak stress state.
 *
 * @param state Pointer to the gateway state structure.
 * @return true if peak stress is active, false otherwise (or on mutex timeout).
 */
bool shared_state_get_peak_stress(gateway_state_t *state);

#endif /* CORE_SHARED_STATE_H */
