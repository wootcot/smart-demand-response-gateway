/**
 * @file smoothing.c
 * @brief Rolling average computation for CT clamp power readings.
 *
 * Implements a 3-second rolling average using a circular buffer of 30 samples
 * (at 100ms sampling intervals). The algorithm handles buffer wraparound
 * transparently and skips failed reads by design — the caller simply does not
 * invoke smoothing_update() when an ADC read fails, preserving buffer integrity.
 *
 * This module is intentionally pure C with no ESP-IDF or FreeRTOS dependencies,
 * enabling host-based unit testing and property-based testing with the theft library.
 */

#include "features/sensor/smoothing.h"

float smoothing_update(float *buffer, uint8_t *index, uint8_t capacity, float new_sample)
{
    if (buffer == NULL || index == NULL || capacity == 0) {
        return 0.0f;
    }

    /* Write the new sample at the current index position in the circular buffer.
     * This overwrites the oldest sample once the buffer has been fully populated,
     * maintaining a fixed-size sliding window without dynamic memory allocation. */
    buffer[*index] = new_sample;

    /* Advance the write index with modulo wraparound. When *index reaches
     * capacity, it wraps back to 0, reusing the oldest slot in the buffer. */
    *index = (*index + 1) % capacity;

    /* Compute the arithmetic mean over the entire buffer. After the buffer fills
     * completely (30 iterations), this always averages exactly `capacity` values.
     * During the initial fill phase, unfilled slots contain 0.0f from initialization,
     * which slightly depresses the average — acceptable for the brief startup transient. */
    float sum = 0.0f;
    for (uint8_t i = 0; i < capacity; i++) {
        sum += buffer[i];
    }

    return sum / (float)capacity;
}

float smoothing_get_average(const float *buffer, uint8_t capacity, uint8_t count)
{
    if (buffer == NULL || count == 0 || capacity == 0) {
        return 0.0f;
    }

    /* Clamp count to capacity to prevent out-of-bounds reads if the caller
     * passes a count exceeding the actual buffer size. */
    uint8_t effective_count = (count > capacity) ? capacity : count;

    float sum = 0.0f;
    for (uint8_t i = 0; i < effective_count; i++) {
        sum += buffer[i];
    }

    return sum / (float)effective_count;
}
