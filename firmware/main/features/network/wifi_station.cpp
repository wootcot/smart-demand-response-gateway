/**
 * @file wifi_station.cpp
 * @brief Wi-Fi station mode initialization and connection management.
 *
 * Brings up the lwIP TCP/IP stack and connects to the configured access point.
 * Uses FreeRTOS event groups to synchronize — the calling task blocks until
 * either an IP is acquired or the connection timeout expires.
 *
 * Auto-reconnect on disconnect is handled via the WIFI_EVENT_STA_DISCONNECTED
 * handler, which retries up to WIFI_MAX_RETRY times before signaling failure.
 */

#include "features/network/wifi_station.hpp"
#include "core/config.h"

#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#include <cstring>

static const char *TAG = "wifi_sta";

static EventGroupHandle_t s_wifi_event_group = nullptr;

static constexpr EventBits_t WIFI_CONNECTED_BIT = BIT0;
static constexpr EventBits_t WIFI_FAIL_BIT      = BIT1;

static int s_retry_count = 0;

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_count < WIFI_MAX_RETRY) {
            esp_wifi_connect();
            s_retry_count++;
            ESP_LOGW(TAG, "Retrying connection (%d/%d)", s_retry_count, WIFI_MAX_RETRY);
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
            ESP_LOGE(TAG, "Connection failed after %d attempts", WIFI_MAX_RETRY);
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        auto *event = static_cast<ip_event_got_ip_t *>(event_data);
        ESP_LOGI(TAG, "Connected — IP: " IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_count = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

bool wifi_init_sta()
{
    s_wifi_event_group = xEventGroupCreate();
    if (s_wifi_event_group == nullptr) {
        ESP_LOGE(TAG, "Failed to create event group");
        return false;
    }

    // Initialize the underlying TCP/IP stack (lwIP)
    ESP_ERROR_CHECK(esp_netif_init());

    // Create the default event loop (required by Wi-Fi and IP events)
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Create the default Wi-Fi station network interface
    esp_netif_create_default_wifi_sta();

    // Initialize Wi-Fi driver with default configuration
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // Register event handlers
    esp_event_handler_instance_t instance_any_wifi;
    esp_event_handler_instance_t instance_got_ip;

    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, nullptr, &instance_any_wifi));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, nullptr, &instance_got_ip));

    // Configure credentials
    wifi_config_t wifi_config = {};
    strlcpy(reinterpret_cast<char *>(wifi_config.sta.ssid), WIFI_SSID,
            sizeof(wifi_config.sta.ssid));
    strlcpy(reinterpret_cast<char *>(wifi_config.sta.password), WIFI_PASS,
            sizeof(wifi_config.sta.password));
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Wi-Fi station started, waiting for connection...");

    // Block until connected or failed
    EventBits_t bits = xEventGroupWaitBits(
        s_wifi_event_group,
        WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
        pdFALSE,
        pdFALSE,
        pdMS_TO_TICKS(WIFI_CONNECT_TIMEOUT_MS));

    // Cleanup event handler instances (events still fire, but we no longer need
    // the blocking synchronization after this point — reconnect is automatic)
    esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_wifi);
    esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip);
    vEventGroupDelete(s_wifi_event_group);
    s_wifi_event_group = nullptr;

    if (bits & WIFI_CONNECTED_BIT) {
        return true;
    }

    ESP_LOGE(TAG, "Wi-Fi connection timed out or failed");
    return false;
}
