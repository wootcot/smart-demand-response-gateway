/**
 * @file relay_control.h
 * @brief Public API for the opto-isolated relay control module.
 *
 * Provides GPIO-based control of an opto-isolated relay array used for
 * 230V AC load shedding. The relay module is driven by the network task
 * when peak-stress instructions are received from the backend server.
 *
 * Hardware design uses opto-isolation (e.g., PC817 optocoupler) between
 * the ESP32's 3.3V GPIO output and the relay coil driver to prevent
 * electrical noise from the 230V AC switching from coupling back into
 * the microcontroller's analog measurement circuits.
 */

#ifndef FEATURES_RELAY_RELAY_CONTROL_H
#define FEATURES_RELAY_RELAY_CONTROL_H

#include <stdbool.h>
#include "esp_err.h"

/**
 * @brief Initialize relay control GPIO pins as outputs.
 *
 * Configures the relay GPIO pins with push-pull output mode and sets
 * initial state to OFF (relays de-energized, loads connected). Must be
 * called from app_main() before the network task begins issuing commands.
 *
 * @return ESP_OK on success, or an error if GPIO configuration fails.
 */
esp_err_t relay_control_init(void);

/**
 * @brief Execute load shedding by energizing relay coils.
 *
 * Activates the opto-isolated relay array to disconnect non-critical
 * loads from the 230V AC mains. Called by the network task when the
 * backend issues a peak_stress_active=true instruction.
 *
 * Relay activation order follows appliance priority: lowest-priority
 * loads are shed first (water heater > space heater > EV charger).
 */
void relay_shed_loads(void);

/**
 * @brief Restore all loads by de-energizing relay coils.
 *
 * Deactivates the relay array to reconnect all loads to the mains.
 * Called when peak stress condition clears (peak_stress_active=false).
 *
 * Restoration uses a staggered delay between relays to prevent
 * simultaneous inrush current from all reconnected appliances.
 */
void relay_restore_loads(void);

/**
 * @brief Query current shedding state.
 *
 * @return true if loads are currently shed (relays energized), false otherwise.
 */
bool relay_is_shedding(void);

#endif /* FEATURES_RELAY_RELAY_CONTROL_H */
