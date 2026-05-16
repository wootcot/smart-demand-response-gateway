/**
 * @file sensor_task.h
 * @brief Public API for the sensor sampling FreeRTOS task.
 *
 * Declares the sensor task entry point function that is created by app_main()
 * and pinned to Core 1. The task performs continuous ADC sampling of the CT clamp
 * signal at 100ms intervals, applies rolling average smoothing, and writes the
 * smoothed value to the shared state for consumption by the network task.
 *
 * Core 1 pinning ensures the sensor sampling loop runs with deterministic timing,
 * isolated from network I/O operations on Core 0 that could introduce jitter
 * through WiFi interrupt handling or TCP retransmission delays.
 */

#ifndef FEATURES_SENSOR_SENSOR_TASK_H
#define FEATURES_SENSOR_SENSOR_TASK_H

#include "core/shared_state.h"

/**
 * @brief FreeRTOS task function for continuous CT clamp ADC sampling.
 *
 * Configures the ADC oneshot driver for 12-bit resolution with DB_11 attenuation,
 * then enters an infinite loop reading the ADC at SAMPLE_INTERVAL_MS intervals.
 * Each successful read updates the smoothing window; failed reads are skipped
 * without altering the buffer. The smoothed average is written to shared state
 * under mutex protection.
 *
 * This function signature matches the FreeRTOS TaskFunction_t prototype.
 * The pvParameters argument must be a pointer to an initialized gateway_state_t.
 *
 * @param pvParameters Pointer to gateway_state_t (cast from void*).
 */
void Task_ReadSensors(void *pvParameters);

#endif /* FEATURES_SENSOR_SENSOR_TASK_H */
