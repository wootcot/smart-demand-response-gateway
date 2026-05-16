/**
 * @file relay_control.c
 * @brief Opto-isolated relay array control for 230V AC load shedding.
 *
 * Implements GPIO-driven relay control for disconnecting non-critical
 * household appliances during peak grid stress. Uses opto-isolation
 * (PC817 or equivalent) between the ESP32 GPIO and relay coil driver
 * to maintain electrical separation between the 3.3V logic domain and
 * the 230V AC power domain.
 *
 * Relay GPIO assignments target ESP32 output-capable pins that do not
 * conflict with the ADC input (GPIO34) or boot-strapping pins. Active-high
 * logic: GPIO HIGH energizes the optocoupler LED, which saturates the
 * phototransistor and drives the relay coil via a MOSFET/BJT stage.
 */

#include "features/relay/relay_control.h"
#include "core/config.h"

#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "relay_control";

/* Relay GPIO pin assignments. These pins are chosen because they are:
 * - Output-capable (not input-only like GPIO34-39)
 * - Not used for boot strapping (avoiding GPIO0, GPIO2, GPIO12, GPIO15)
 * - Not conflicting with UART0 (GPIO1, GPIO3) used for serial monitor
 * Active-high: setting GPIO HIGH energizes the relay via optocoupler. */
#define RELAY_PIN_1   GPIO_NUM_25   /* Priority 1: Water heater (highest shed priority) */
#define RELAY_PIN_2   GPIO_NUM_26   /* Priority 2: Space heater */
#define RELAY_PIN_3   GPIO_NUM_27   /* Priority 3: EV charger / pump motor */

/* Number of relay channels in the array. */
#define RELAY_COUNT   3

/* Staggered restoration delay between relay de-energizations (milliseconds).
 * Prevents simultaneous inrush current from all reconnected motor-driven
 * appliances which could trigger the very overload we're trying to prevent. */
#define RESTORE_STAGGER_MS  500

/* Track current shedding state to avoid redundant GPIO writes and to
 * support the relay_is_shedding() query from other modules. */
static bool s_is_shedding = false;

/* Array of relay GPIO pins ordered by shedding priority (first shed, last restored). */
static const gpio_num_t relay_pins[RELAY_COUNT] = {
    RELAY_PIN_1,
    RELAY_PIN_2,
    RELAY_PIN_3,
};

esp_err_t relay_control_init(void)
{
    /* Configure all relay pins as push-pull outputs with initial LOW state.
     * LOW = relay de-energized = load connected (fail-safe: power loss
     * reconnects all loads, preventing indefinite shedding on MCU crash). */
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << RELAY_PIN_1) |
                        (1ULL << RELAY_PIN_2) |
                        (1ULL << RELAY_PIN_3),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    esp_err_t err = gpio_config(&io_conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure relay GPIOs: %s", esp_err_to_name(err));
        return err;
    }

    /* Ensure all relays start de-energized (loads connected). */
    for (int i = 0; i < RELAY_COUNT; i++) {
        gpio_set_level(relay_pins[i], 0);
    }

    s_is_shedding = false;
    ESP_LOGI(TAG, "Relay control initialized: %d channels, all loads connected", RELAY_COUNT);
    return ESP_OK;
}

void relay_shed_loads(void)
{
    if (s_is_shedding) {
        return; /* Already shedding — avoid redundant GPIO transitions. */
    }

    ESP_LOGW(TAG, "Peak stress: shedding %d load(s) in priority order", RELAY_COUNT);

    /* Shed loads in priority order: lowest-priority appliance first.
     * This ensures the most dispensable loads are disconnected before
     * touching higher-priority circuits. */
    for (int i = 0; i < RELAY_COUNT; i++) {
        gpio_set_level(relay_pins[i], 1);
        ESP_LOGI(TAG, "Relay %d ENERGIZED (load shed)", i + 1);
    }

    s_is_shedding = true;
}

void relay_restore_loads(void)
{
    if (!s_is_shedding) {
        return; /* Already restored — no action needed. */
    }

    ESP_LOGI(TAG, "Peak stress cleared: restoring %d load(s) with stagger", RELAY_COUNT);

    /* Restore loads in reverse priority order (highest priority first)
     * with a staggered delay between each reconnection. The stagger
     * prevents simultaneous inrush current from multiple motor-driven
     * appliances which could re-trigger grid overload. */
    for (int i = RELAY_COUNT - 1; i >= 0; i--) {
        gpio_set_level(relay_pins[i], 0);
        ESP_LOGI(TAG, "Relay %d DE-ENERGIZED (load restored)", i + 1);

        if (i > 0) {
            vTaskDelay(pdMS_TO_TICKS(RESTORE_STAGGER_MS));
        }
    }

    s_is_shedding = false;
}

bool relay_is_shedding(void)
{
    return s_is_shedding;
}
