/**
 * @file nvs_config.cpp
 * @brief NVS-backed runtime configuration implementation.
 *
 * Reads credentials from two NVS namespaces:
 *   - "wifi"    → ssid, password
 *   - "gateway" → ws_host, ws_port, gw_id
 *
 * If a key is not found in NVS, the corresponding getter returns the
 * Kconfig compile-time default (CONFIG_GATEWAY_*).
 */

#include "core/nvs_config.hpp"

#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include "sdkconfig.h"

#include <cstring>

static const char *TAG = "nvs_config";

bool NvsConfig::load()
{
    nvs_handle_t handle;
    esp_err_t err;

    // --- Wi-Fi namespace ---
    err = nvs_open("wifi", NVS_READONLY, &handle);
    if (err == ESP_OK) {
        size_t len = sizeof(ssid_);
        if (nvs_get_str(handle, "ssid", ssid_, &len) == ESP_OK) {
            ssid_loaded_ = true;
            ESP_LOGI(TAG, "Wi-Fi SSID loaded from NVS");
        }

        len = sizeof(pass_);
        if (nvs_get_str(handle, "password", pass_, &len) == ESP_OK) {
            pass_loaded_ = true;
            ESP_LOGI(TAG, "Wi-Fi password loaded from NVS");
        }

        nvs_close(handle);
    } else if (err == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGI(TAG, "No 'wifi' NVS namespace — using Kconfig defaults");
    } else {
        ESP_LOGW(TAG, "Failed to open 'wifi' NVS namespace: %s", esp_err_to_name(err));
    }

    // --- Gateway namespace ---
    err = nvs_open("gateway", NVS_READONLY, &handle);
    if (err == ESP_OK) {
        size_t len = sizeof(ws_host_);
        if (nvs_get_str(handle, "ws_host", ws_host_, &len) == ESP_OK) {
            ws_host_loaded_ = true;
            ESP_LOGI(TAG, "WebSocket host loaded from NVS");
        }

        uint16_t port = 0;
        if (nvs_get_u16(handle, "ws_port", &port) == ESP_OK) {
            ws_port_ = port;
            ws_port_loaded_ = true;
            ESP_LOGI(TAG, "WebSocket port loaded from NVS: %u", port);
        }

        len = sizeof(gw_id_);
        if (nvs_get_str(handle, "gw_id", gw_id_, &len) == ESP_OK) {
            gw_id_loaded_ = true;
            ESP_LOGI(TAG, "Gateway ID loaded from NVS");
        }

        nvs_close(handle);
    } else if (err == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGI(TAG, "No 'gateway' NVS namespace — using Kconfig defaults");
    } else {
        ESP_LOGW(TAG, "Failed to open 'gateway' NVS namespace: %s", esp_err_to_name(err));
    }

    return true;
}

const char* NvsConfig::wifi_ssid() const
{
    return ssid_loaded_ ? ssid_ : CONFIG_GATEWAY_WIFI_SSID;
}

const char* NvsConfig::wifi_pass() const
{
    return pass_loaded_ ? pass_ : CONFIG_GATEWAY_WIFI_PASS;
}

const char* NvsConfig::ws_host() const
{
    return ws_host_loaded_ ? ws_host_ : CONFIG_GATEWAY_WS_HOST;
}

uint16_t NvsConfig::ws_port() const
{
    return ws_port_loaded_ ? ws_port_ : CONFIG_GATEWAY_WS_PORT;
}

const char* NvsConfig::gateway_id() const
{
    return gw_id_loaded_ ? gw_id_ : CONFIG_GATEWAY_ID;
}
