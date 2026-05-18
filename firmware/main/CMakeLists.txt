# Register the main component with explicit source files and ESP-IDF dependencies.
# Using explicit file list (no GLOB) ensures CMake correctly detects source changes
# and prevents accidental inclusion of test or scratch files.
idf_component_register(
    SRCS
        "main.cpp"
        "core/gateway_state.cpp"
        "features/sensor/current_sensor.cpp"
        "features/network/network_client.cpp"
        "features/network/wifi_station.cpp"
        "features/relay/relay_controller.cpp"
    INCLUDE_DIRS
        "."
    REQUIRES
        driver
        esp_driver_gpio
        esp_adc
        freertos
        esp_websocket_client
        esp_event
        esp_wifi
        esp_netif
        nvs_flash
        log
)
