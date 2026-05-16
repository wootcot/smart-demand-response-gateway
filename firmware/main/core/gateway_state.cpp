/**
 * @file gateway_state.cpp
 * @brief Mutex-protected shared state implementation for dual-core task communication.
 *
 * Uses FreeRTOS mutex primitives with a 50ms acquisition timeout to prevent
 * indefinite blocking — if a task cannot acquire the mutex within 50ms, the
 * operation is skipped rather than stalling the real-time sampling loop.
 */

#include "core/gateway_state.hpp"

bool GatewayState::init() noexcept
{
    mutex = xSemaphoreCreateMutex();
    return (mutex != nullptr);
}

bool GatewayState::write_power(float watts) noexcept
{
    if (xSemaphoreTake(mutex, MUTEX_TIMEOUT) != pdTRUE) {
        return false;
    }

    current_avg_watts = watts;

    xSemaphoreGive(mutex);
    return true;
}

float GatewayState::read_power() noexcept
{
    if (xSemaphoreTake(mutex, MUTEX_TIMEOUT) != pdTRUE) {
        // Return 0.0 on timeout rather than stale data — the network task
        // will simply report zero and retry on the next 10s interval.
        return 0.0f;
    }

    float watts = current_avg_watts;

    xSemaphoreGive(mutex);
    return watts;
}

bool GatewayState::set_peak_stress(bool active) noexcept
{
    if (xSemaphoreTake(mutex, MUTEX_TIMEOUT) != pdTRUE) {
        return false;
    }

    peak_stress_active = active;

    xSemaphoreGive(mutex);
    return true;
}

bool GatewayState::get_peak_stress() noexcept
{
    if (xSemaphoreTake(mutex, MUTEX_TIMEOUT) != pdTRUE) {
        // Default to false on timeout — fail-safe keeps loads connected
        // rather than shedding on a transient mutex contention.
        return false;
    }

    bool active = peak_stress_active;

    xSemaphoreGive(mutex);
    return active;
}
