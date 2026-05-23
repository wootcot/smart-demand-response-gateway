/**
 * @file nvs_config.hpp
 * @brief NVS-backed runtime configuration for Wi-Fi and backend credentials.
 *
 * Provides a two-tier configuration strategy:
 *   1. NVS (runtime) — Per-device credentials written via provisioning tool
 *   2. Kconfig (build-time) — Fallback defaults compiled into the binary
 *
 * This allows flashing the same firmware binary to multiple gateways while
 * provisioning unique credentials per device without rebuilding.
 *
 * Usage:
 *   NvsConfig cfg;
 *   cfg.load();  // Call after nvs_flash_init()
 *   cfg.wifi_ssid();  // Returns NVS value if set, else Kconfig default
 */

#pragma once

#include <cstdint>

/// Maximum lengths for NVS string values (including null terminator)
inline constexpr size_t NVS_WIFI_SSID_MAX = 33;   // 802.11 max SSID = 32 chars
inline constexpr size_t NVS_WIFI_PASS_MAX = 65;   // WPA2 max = 63 chars
inline constexpr size_t NVS_WS_HOST_MAX   = 128;
inline constexpr size_t NVS_GATEWAY_ID_MAX = 32;

/**
 * @brief Runtime configuration loaded from NVS with Kconfig fallback.
 *
 * All getters return a valid null-terminated string — either the NVS value
 * (if provisioned) or the Kconfig compile-time default.
 */
class NvsConfig {
public:
    /**
     * @brief Load configuration from NVS.
     *
     * Reads all keys from the "wifi" and "gateway" NVS namespaces.
     * Missing keys are silently ignored (Kconfig defaults apply).
     * Must be called after nvs_flash_init().
     *
     * @return true if NVS was opened successfully (even if no keys found).
     */
    bool load();

    /// Wi-Fi SSID (NVS key: "wifi/ssid")
    const char* wifi_ssid() const;

    /// Wi-Fi password (NVS key: "wifi/password")
    const char* wifi_pass() const;

    /// WebSocket server host (NVS key: "gateway/ws_host")
    const char* ws_host() const;

    /// WebSocket server port (NVS key: "gateway/ws_port")
    uint16_t ws_port() const;

    /// Gateway device ID (NVS key: "gateway/gw_id")
    const char* gateway_id() const;

private:
    char ssid_[NVS_WIFI_SSID_MAX]       = {};
    char pass_[NVS_WIFI_PASS_MAX]       = {};
    char ws_host_[NVS_WS_HOST_MAX]      = {};
    char gw_id_[NVS_GATEWAY_ID_MAX]     = {};
    uint16_t ws_port_                    = 0;

    bool ssid_loaded_    = false;
    bool pass_loaded_    = false;
    bool ws_host_loaded_ = false;
    bool ws_port_loaded_ = false;
    bool gw_id_loaded_   = false;
};
