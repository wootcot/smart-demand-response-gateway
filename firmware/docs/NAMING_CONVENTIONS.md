# Naming Conventions

Firmware naming conventions for the Smart Demand Response Gateway project.
These align with industry-standard embedded C++ practices (Google C++ Style,
ESP-IDF ecosystem, MISRA-adjacent embedded teams).

## Summary Table

| Element              | Convention       | Example                        |
| -------------------- | ---------------- | ------------------------------ |
| Classes / Structs    | PascalCase       | `RelayController`              |
| Methods / Functions  | snake_case       | `shed_loads()`, `read_power()` |
| Local variables      | snake_case       | `raw_value`, `current_amps`    |
| Private members      | snake_case       | `is_shedding`, `adc_handle`    |
| Constants / Defines  | UPPER_SNAKE      | `RELAY_COUNT`, `ADC_UNIT_ID`   |
| Hardware types (\_t) | snake_case + \_t | `gpio_num_t`, `adc_channel_t`  |
| File names           | snake_case       | `relay_controller.cpp`         |
| FreeRTOS task names  | PascalCase       | `Task_ReadSensors`             |

## Rationale

The goal is instant visual recognition of what you're looking at:

- **PascalCase class** → application-level wrapper we wrote.
- **snake_case function** → could be ours or SDK; context (class prefix or free function) disambiguates.
- **UPPER_SNAKE constant** → compile-time configuration value.
- **`_t` suffix type** → hardware/SDK typedef from ESP-IDF, POSIX, or FreeRTOS.

This avoids camelCase for methods/variables because the entire ESP-IDF and
FreeRTOS API surface is snake_case. Mixing camelCase application code with
snake_case SDK calls creates visual noise rather than reducing it.

## Optional: `m_` Prefix for Private Members

Teams that want stronger "this is internal state" signaling may adopt:

```cpp
class RelayController {
private:
    bool m_is_shedding = false;
};
```

This is common in Qt, automotive embedded, and MISRA-adjacent codebases.
Not currently enforced in this project — adopt if the team agrees.

## References

- [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html)
- [ESP-IDF Coding Style](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/contribute/style-guide.html)
- [MISRA C++ 2023 Guidelines](https://misra.org.uk/)
