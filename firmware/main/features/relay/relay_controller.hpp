/**
 * @file relay_controller.hpp
 * @brief Opto-isolated relay array controller for 230V AC load shedding.
 *
 * Encapsulates GPIO-driven relay control for disconnecting non-critical
 * household appliances during peak grid stress. Uses opto-isolation
 * (PC817 or equivalent) between the ESP32 GPIO and relay coil driver
 * to maintain electrical separation between the 3.3V logic domain and
 * the 230V AC power domain.
 *
 * Active-high logic: GPIO HIGH energizes the optocoupler LED, which
 * saturates the phototransistor and drives the relay coil via MOSFET/BJT.
 */

#pragma once

#include "driver/gpio.h"
#include "esp_err.h"
#include "core/config.h"

class RelayController {
private:
    bool is_shedding = false;

    static constexpr gpio_num_t pins[RELAY_COUNT] = {
        RELAY_PIN_1,
        RELAY_PIN_2,
        RELAY_PIN_3,
    };

public:
    RelayController() = default;
    ~RelayController() = default;

    // Non-copyable (owns hardware resource)
    RelayController(const RelayController&) = delete;
    RelayController& operator=(const RelayController&) = delete;

    // Configure relay GPIOs as push-pull outputs, initial state LOW (loads connected).
    // Must be called before shed_loads() or restore_loads().
    bool init() noexcept;

    // Energize all relays in priority order to disconnect non-critical loads.
    void shed_loads() noexcept;

    // De-energize relays in reverse priority order with staggered delay
    // to prevent simultaneous inrush current.
    void restore_loads() noexcept;

    // Query whether loads are currently shed.
    bool shedding() const noexcept { return is_shedding; }
};
