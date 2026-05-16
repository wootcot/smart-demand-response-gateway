/**
 * @file shared_state.c
 * @brief Mutex-protected shared state implementation for dual-core task communication.
 *
 * Implements the gateway_state_t lifecycle: initialization, thread-safe sample
 * writes from the sensor task, and thread-safe average reads from the network task.
 * Uses FreeRTOS mutex primitives with a 50ms acquisition timeout to prevent
 * indefinite blocking — if a task cannot acquire the mutex within 50ms, the
 * operation is skipped rather than stalling the real-time sampling loop.
 */

#include "core/shared_state.h"
#include <string.h>

/* Mutex acquisition timeout in ticks. 50ms is short enough to avoid disrupting
 * the 100ms sensor sampling cadence while allowing for brief contention. */
#define MUTEX_TIMEOUT_TICKS  pdMS_TO_TICKS(50)

esp_err_t shared_state_init(gateway_state_t *state)
{
    /* Zero the entire smoothing buffer so initial average reads return 0.0
     * rather than undefined floating-point values. */
    memset(state->smoothing_buffer, 0, sizeof(state->smoothing_buffer));
    state->buffer_index = 0;
    state->current_avg_watts = 0.0f;

    state->mutex = xSemaphoreCreateMutex();
    if (state->mutex == NULL) {
        return ESP_ERR_NO_MEM;
    }

    return ESP_OK;
}

esp_err_t shared_state_write_sample(gateway_state_t *state, float sample)
{
    if (xSemaphoreTake(state->mutex, MUTEX_TIMEOUT_TICKS) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    /* Write sample into circular buffer at current index, then advance
     * with modulo wraparound to reuse oldest slot. */
    state->smoothing_buffer[state->buffer_index] = sample;
    state->buffer_index = (state->buffer_index + 1) % SMOOTHING_SAMPLES;

    /* Recompute rolling average over the full buffer. After the buffer fills
     * (30 samples = 3 seconds), this always averages exactly 30 values.
     * Before the buffer fills, zeros in unfilled slots pull the average down,
     * which is acceptable during the brief startup transient. */
    float sum = 0.0f;
    for (uint8_t i = 0; i < SMOOTHING_SAMPLES; i++) {
        sum += state->smoothing_buffer[i];
    }
    state->current_avg_watts = sum / (float)SMOOTHING_SAMPLES;

    xSemaphoreGive(state->mutex);
    return ESP_OK;
}

float shared_state_read_average(gateway_state_t *state)
{
    float avg = 0.0f;

    if (xSemaphoreTake(state->mutex, MUTEX_TIMEOUT_TICKS) != pdTRUE) {
        /* Return 0.0 on timeout rather than stale data — the network task
         * will simply report zero and retry on the next 10s interval. */
        return 0.0f;
    }

    avg = state->current_avg_watts;

    xSemaphoreGive(state->mutex);
    return avg;
}
