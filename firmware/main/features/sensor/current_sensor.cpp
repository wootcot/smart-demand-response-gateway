/**
 * @file current_sensor.cpp
 * @brief CT clamp current sensing via ADS1115 16-bit ADC over I2C.
 *
 * Uses the ESP-IDF v5.x I2C master driver to communicate with the ADS1115
 * in continuous conversion mode at 860 SPS. Each measurement cycle reads
 * ~430 samples over 500ms (25 full AC cycles at 50Hz), then computes the
 * root-mean-square value after subtracting the DC bias midpoint.
 *
 * Two-stage smoothing strategy:
 *   1. RMS over 25 AC cycles — correctly handles sinusoidal waveform and
 *      inherently rejects short startup inrush surges (1-5 cycles) by
 *      averaging their energy contribution across the full measurement window.
 *   2. Rolling average of 6 consecutive RMS readings (3s total) — further
 *      suppresses motor inrush transients lasting 1-2 seconds.
 *
 * This ensures the gateway does not misinterpret a safe startup surge as
 * an emergency grid stress overload event.
 */

#include "features/sensor/current_sensor.hpp"
#include "esp_log.h"
#include "freertos/FreeRTOS.h" // IWYU pragma: keep
#include "freertos/task.h"

#include <cmath>

static const char *TAG = "current_sensor";

CurrentSensor::CurrentSensor(float cal_factor) noexcept
    : calibration_factor(cal_factor)
{
}

CurrentSensor::~CurrentSensor()
{
    if (dev_handle) {
        i2c_master_bus_rm_device(dev_handle);
    }
    if (bus_handle) {
        i2c_del_master_bus(bus_handle);
    }
}

bool CurrentSensor::write_register(uint8_t reg, uint16_t value) noexcept
{
    // ADS1115 expects: [register_pointer, MSB, LSB]
    uint8_t buf[3] = {
        reg,
        static_cast<uint8_t>((value >> 8) & 0xFF),
        static_cast<uint8_t>(value & 0xFF)
    };

    esp_err_t ret = i2c_master_transmit(dev_handle, buf, sizeof(buf), 100);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C write reg 0x%02X failed: %s", reg, esp_err_to_name(ret));
        return false;
    }
    return true;
}

bool CurrentSensor::read_register(uint8_t reg, int16_t &value) noexcept
{
    // Set register pointer, then read 2 bytes
    uint8_t reg_addr = reg;
    uint8_t buf[2] = {0};

    esp_err_t ret = i2c_master_transmit_receive(
        dev_handle, &reg_addr, 1, buf, 2, 100);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C read reg 0x%02X failed: %s", reg, esp_err_to_name(ret));
        return false;
    }

    // ADS1115 returns MSB first (big-endian)
    value = static_cast<int16_t>((buf[0] << 8) | buf[1]);
    return true;
}

bool CurrentSensor::init() noexcept
{
    // Configure I2C master bus
    i2c_master_bus_config_t bus_cfg = {};
    bus_cfg.i2c_port = I2C_NUM_0;
    bus_cfg.sda_io_num = I2C_SDA_PIN;
    bus_cfg.scl_io_num = I2C_SCL_PIN;
    bus_cfg.clk_source = I2C_CLK_SRC_DEFAULT;
    bus_cfg.glitch_ignore_cnt = 7;
    bus_cfg.flags.enable_internal_pullup = true;

    esp_err_t ret = i2c_new_master_bus(&bus_cfg, &bus_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create I2C master bus: %s", esp_err_to_name(ret));
        return false;
    }

    // Add ADS1115 as a device on the bus
    i2c_device_config_t dev_cfg = {};
    dev_cfg.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    dev_cfg.device_address = ADS1115_ADDR;
    dev_cfg.scl_speed_hz = I2C_FREQ_HZ;

    ret = i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add ADS1115 device: %s", esp_err_to_name(ret));
        i2c_del_master_bus(bus_handle);
        bus_handle = nullptr;
        return false;
    }

    // Configure ADS1115 for continuous conversion on AIN0 at 860 SPS
    if (!write_register(ADS1115_REG_CONFIG, ADS1115_CONFIG_CONTINUOUS)) {
        ESP_LOGE(TAG, "Failed to configure ADS1115");
        return false;
    }

    // Allow first conversion to complete (~1.2ms at 860 SPS)
    vTaskDelay(pdMS_TO_TICKS(5));

    // Verify communication by reading back config register
    int16_t config_readback = 0;
    if (!read_register(ADS1115_REG_CONFIG, config_readback)) {
        ESP_LOGE(TAG, "Failed to verify ADS1115 config");
        return false;
    }

    ESP_LOGI(TAG, "ADS1115 initialized: addr=0x%02X, SCL=GPIO%d, SDA=GPIO%d, "
             "860 SPS continuous, PGA=±4.096V, cal_factor=%.1f A/V, "
             "RMS window=%d cycles (%dms), smoothing=%d readings",
             ADS1115_ADDR, I2C_SCL_PIN, I2C_SDA_PIN,
             calibration_factor,
             RMS_CYCLE_COUNT, RMS_WINDOW_DURATION_MS, SMOOTHING_WINDOW_SIZE);
    return true;
}

float CurrentSensor::compute_rms_amps() noexcept
{
    // Read the ADS1115 conversion register repeatedly over the RMS window.
    // At 860 SPS in continuous mode, each read returns the latest completed
    // conversion. We pace reads at ~RMS_SAMPLE_INTERVAL_US to avoid reading
    // the same conversion twice while maximizing sample count.
    //
    // The RMS calculation squares each sample, accumulates the sum, then takes
    // the square root of the mean. This correctly handles the sinusoidal AC
    // waveform — a short inrush spike lasting 1-5 cycles contributes only a
    // fraction of its energy to the 25-cycle RMS window, preventing false
    // overload detection.

    float sum_of_squares = 0.0f;
    uint16_t valid_samples = 0;

    for (uint16_t i = 0; i < RMS_TOTAL_SAMPLES; i++) {
        int16_t raw_value = 0;
        bool ok = read_register(ADS1115_REG_CONVERSION, raw_value);

        if (ok) {
            // Convert raw ADC count to voltage
            float voltage = static_cast<float>(raw_value) * ADS1115_LSB_VOLTAGE;

            // Subtract DC bias midpoint to get the AC component
            float ac_voltage = voltage - DC_BIAS_MIDPOINT_V;

            // Convert AC voltage to amperes via calibration factor
            // (CT ratio: 30A produces 1V across 33Ω burden)
            float amps = ac_voltage * calibration_factor;

            sum_of_squares += amps * amps;
            valid_samples++;
        }
        // Failed reads are silently skipped — we only need a majority of
        // samples for an accurate RMS estimate.

        // Busy-wait for the inter-sample interval (~1163µs at 860 SPS).
        // esp_rom_delay_us provides microsecond-precision delay without
        // yielding to the scheduler, maintaining deterministic sample timing.
        if (i < RMS_TOTAL_SAMPLES - 1) {
            esp_rom_delay_us(RMS_SAMPLE_INTERVAL_US);
        }
    }

    // Require at least 75% of samples to be valid for a trustworthy RMS reading
    uint16_t min_valid = (RMS_TOTAL_SAMPLES * 3) / 4;
    if (valid_samples < min_valid) {
        ESP_LOGW(TAG, "RMS burst: only %u/%u valid samples (need %u), discarding",
                 valid_samples, RMS_TOTAL_SAMPLES, min_valid);
        return -1.0f;
    }

    float mean_square = sum_of_squares / static_cast<float>(valid_samples);
    float rms_amps = sqrtf(mean_square);

    return rms_amps;
}

float CurrentSensor::read_smoothed_amps() noexcept
{
    float rms_amps = compute_rms_amps();

    if (rms_amps < 0.0f) {
        // Burst measurement failed — return last known rolling average
        // without modifying the smoothing buffer.
        ESP_LOGW(TAG, "RMS measurement failed, returning cached average");
        float sum = 0.0f;
        for (uint8_t i = 0; i < SMOOTHING_WINDOW_SIZE; i++) {
            sum += smoothing_window[i];
        }
        return sum / static_cast<float>(SMOOTHING_WINDOW_SIZE);
    }

    // Write RMS reading into circular buffer at current index
    smoothing_window[window_index] = rms_amps;
    window_index = (window_index + 1) % SMOOTHING_WINDOW_SIZE;

    // Compute arithmetic mean over the smoothing window
    float sum = 0.0f;
    for (uint8_t i = 0; i < SMOOTHING_WINDOW_SIZE; i++) {
        sum += smoothing_window[i];
    }

    float smoothed = sum / static_cast<float>(SMOOTHING_WINDOW_SIZE);

    ESP_LOGD(TAG, "RMS=%.2fA, smoothed=%.2fA", rms_amps, smoothed);
    return smoothed;
}
