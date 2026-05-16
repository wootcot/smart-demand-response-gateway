/**
 * @file relay_controller.cpp
 * @brief Opto-isolated relay array control implementation.
 *
 * Relay GPIO assignments target ESP32 output-capable pins that do not
 * conflict with the ADC input (GPIO34) or boot-strapping pins.
 *
 * Fail-safe design: LOW = relay de-energized = load connected. Power loss
 * reconnects all loads, preventing indefinite shedding on MCU crash.
 */

#include "features/relay/relay_controller.hpp"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "relay_ctrl";

bool RelayController::init() noexcept
{
    gpio_config_t io_conf = {};
    io_conf.pin_bit_mask = (1ULL << RELAY_PIN_1) |
                           (1ULL << RELAY_PIN_2) |
                           (1ULL << RELAY_PIN_3);
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.intr_type = GPIO_INTR_DISABLE;

    esp_err_t err = gpio_config(&io_conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure relay GPIOs: %s", esp_err_to_name(err));
        return false;
    }

    // Ensure all relays start de-energized (loads connected)
    for (int i = 0; i < RELAY_COUNT; i++) {
        gpio_set_level(pins[i], 0);
    }

    is_shedding = false;
    ESP_LOGI(TAG, "Relay control initialized: %d channels, all loads connected", RELAY_COUNT);
    return true;
}

void RelayController::shed_loads() noexcept
{
    if (is_shedding) {
        return;
    }

    ESP_LOGW(TAG, "Peak stress: shedding %d load(s) in priority order", RELAY_COUNT);

    for (int i = 0; i < RELAY_COUNT; i++) {
        gpio_set_level(pins[i], 1);
        ESP_LOGI(TAG, "Relay %d ENERGIZED (load shed)", i + 1);
    }

    is_shedding = true;
}

void RelayController::restore_loads() noexcept
{
    if (!is_shedding) {
        return;
    }

    ESP_LOGI(TAG, "Peak stress cleared: restoring %d load(s) with stagger", RELAY_COUNT);

    // Restore in reverse priority order with staggered delay to prevent
    // simultaneous inrush current from motor-driven appliances
    for (int i = RELAY_COUNT - 1; i >= 0; i--) {
        gpio_set_level(pins[i], 0);
        ESP_LOGI(TAG, "Relay %d DE-ENERGIZED (load restored)", i + 1);

        if (i > 0) {
            vTaskDelay(pdMS_TO_TICKS(RESTORE_STAGGER_MS));
        }
    }

    is_shedding = false;
}
