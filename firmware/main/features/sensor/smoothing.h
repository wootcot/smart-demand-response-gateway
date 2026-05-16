/**
 * @file smoothing.h
 * @brief Pure computation interface for rolling average smoothing of ADC readings.
 *
 * Provides stateless functions for maintaining a circular buffer of power samples
 * and computing their arithmetic mean. These functions are intentionally free of
 * hardware dependencies (no FreeRTOS, no ESP-IDF APIs) so they can be unit-tested
 * on a host machine without cross-compilation or hardware emulation.
 *
 * The smoothing window suppresses inrush current spikes from motor-driven
 * appliances (refrigerators, water pumps) which typically last 1-2 seconds.
 * A 3-second window (30 samples at 100ms intervals) ensures these transients
 * are averaged out before telemetry is transmitted to the backend.
 */

#ifndef FEATURES_SENSOR_SMOOTHING_H
#define FEATURES_SENSOR_SMOOTHING_H

#include <stdint.h>

/**
 * @brief Update the rolling buffer with a new sample and return the new average.
 *
 * Writes new_sample into the circular buffer at the position indicated by *index,
 * advances *index with wraparound at capacity, then computes and returns the
 * arithmetic mean of all buffer entries.
 *
 * @param buffer      Pointer to the float array serving as the circular buffer.
 * @param index       Pointer to the current write index (modified on return).
 * @param capacity    Total number of slots in the buffer (e.g., SMOOTHING_SAMPLES).
 * @param new_sample  The new power reading in watts to insert.
 * @return            Arithmetic mean of all buffer entries after insertion.
 */
float smoothing_update(float *buffer, uint8_t *index, uint8_t capacity, float new_sample);

/**
 * @brief Compute the arithmetic mean of buffer contents.
 *
 * Sums the first `count` entries in the buffer and divides by count.
 * If count is zero, returns 0.0f to avoid division by zero.
 *
 * @param buffer    Pointer to the float array containing samples.
 * @param capacity  Total number of slots in the buffer (used for bounds safety).
 * @param count     Number of valid samples currently in the buffer.
 * @return          Arithmetic mean of the first `count` entries, or 0.0f if count is 0.
 */
float smoothing_get_average(const float *buffer, uint8_t capacity, uint8_t count);

#endif /* FEATURES_SENSOR_SMOOTHING_H */
