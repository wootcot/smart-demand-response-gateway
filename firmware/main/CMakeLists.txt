# Register the main component with explicit source files and ESP-IDF dependencies.
# Using explicit file list (no GLOB) ensures CMake correctly detects source changes
# and prevents accidental inclusion of test or scratch files.
idf_component_register(
    SRCS
        "main.c"
        "core/shared_state.c"
        "features/sensor/smoothing.c"
        "features/sensor/sensor_task.c"
        "features/network/network_task.c"
        "features/relay/relay_control.c"
    INCLUDE_DIRS
        "."
    REQUIRES
        driver
        esp_adc
        freertos
        esp_websocket_client
        esp_event
        log
)
