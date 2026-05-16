/**
 * @file gateway_state.hpp
 * @brief Thread-safe shared state for inter-task communication.
 *
 * Encapsulates the mutex-protected data exchanged between the sensor task
 * (Core 1) and network task (Core 0). Uses FreeRTOS mutex primitives with
 * a 50ms acquisition timeout to prevent indefinite blocking.
 *
 * Design rationale: A mutex-protected class provides direct access semantics
 * appropriate for a single-producer (sensor) / single-consumer (network)
 * pattern with infrequent reads.
 */

#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

class GatewayState {
private:
    float current_avg_watts = 0.0f;
    bool peak_stress_active = false;
    SemaphoreHandle_t mutex = nullptr;

    static constexpr TickType_t MUTEX_TIMEOUT = pdMS_TO_TICKS(50);

public:
    GatewayState() = default;
    ~GatewayState() = default;

    // Non-copyable, non-movable (static lifetime object)
    GatewayState(const GatewayState&) = delete;
    GatewayState& operator=(const GatewayState&) = delete;

    bool init() noexcept;

    // Called by sensor task to publish smoothed power reading
    bool write_power(float watts) noexcept;

    // Called by network task to read current smoothed power
    float read_power() noexcept;

    // Called by network task when backend issues peak-stress instruction
    bool set_peak_stress(bool active) noexcept;

    // Called by network task to query current peak-stress state
    bool get_peak_stress() noexcept;
};
