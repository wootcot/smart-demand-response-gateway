/**
 * @file wifi_station.hpp
 * @brief Wi-Fi station initialization for ESP32.
 *
 * Provides a blocking initialization function that brings up the TCP/IP stack,
 * connects to the configured AP, and waits until an IP address is acquired.
 * Must be called before any network operations (WebSocket, HTTP, DNS).
 */

#pragma once

/**
 * @brief Initialize Wi-Fi in station mode and block until connected.
 *
 * Performs the full initialization sequence:
 *   1. esp_netif_init() — TCP/IP stack
 *   2. esp_event_loop_create_default() — system event loop
 *   3. esp_wifi_init/start — radio bring-up
 *   4. Waits for IP_EVENT_STA_GOT_IP (blocks up to WIFI_CONNECT_TIMEOUT_MS)
 *
 * @param ssid  Wi-Fi network name (null-terminated, max 32 chars)
 * @param pass  Wi-Fi password (null-terminated, max 63 chars)
 * @return true if connected and IP acquired, false on timeout or failure.
 */
bool wifi_init_sta(const char *ssid, const char *pass);
