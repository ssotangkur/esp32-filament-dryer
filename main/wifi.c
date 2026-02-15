#include "wifi.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_sntp.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include <string.h>
#include <time.h>

static const char *TAG = "wifi";

// WiFi event group and bits
static EventGroupHandle_t wifi_event_group;
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

// WiFi retry variables
static int wifi_retry_count = 0;
static const int MAX_WIFI_RETRIES = 5;

// Event handler for WiFi events
static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
  if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
  {
    ESP_LOGI(TAG, "WiFi STA started, connecting to AP...");
    wifi_retry_count = 0; // Reset retry count on start
    esp_wifi_connect();
  }
  else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
  {
    wifi_event_sta_disconnected_t *disconn = (wifi_event_sta_disconnected_t *)event_data;
    ESP_LOGI(TAG, "WiFi disconnected, reason: %d", disconn->reason);

    if (wifi_retry_count < MAX_WIFI_RETRIES)
    {
      wifi_retry_count++;
      int delay_ms = (1 << (wifi_retry_count - 1)) * 1000; // Exponential backoff: 1s, 2s, 4s, 8s, 16s
      ESP_LOGI(TAG, "Retrying WiFi connection in %d ms (attempt %d/%d)", delay_ms, wifi_retry_count, MAX_WIFI_RETRIES);
      vTaskDelay(pdMS_TO_TICKS(delay_ms));
      esp_wifi_connect();
    }
    else
    {
      ESP_LOGI(TAG, "Max WiFi retries exceeded, giving up");
      xEventGroupSetBits(wifi_event_group, WIFI_FAIL_BIT);
    }
  }
  else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
  {
    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
    ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
    wifi_retry_count = 0; // Reset retry count on successful connection
    xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);

    // Sync time from NTP (DHCP or fallback servers)
    wifi_sync_time();
  }
}

// Sync time using NTP with fallback servers
void wifi_sync_time(void)
{
  ESP_LOGI(TAG, "Starting NTP time sync...");

  // Configure NTP servers - try multiple public NTP servers for reliability
  // Using pool.ntp.org anycast servers which are widely available
  esp_sntp_setservername(0, "pool.ntp.org");
  esp_sntp_setservername(1, "time.google.com");
  esp_sntp_setservername(2, "time.cloudflare.com");

  ESP_LOGI(TAG, "NTP servers configured: pool.ntp.org, time.google.com, time.cloudflare.com");

  // Initialize SNTP
  esp_sntp_init();

  // Wait for time sync with timeout
  time_t now = 0;
  struct tm timeinfo = {0};
  int retry_count = 0;
  const int max_retries = 20;

  while (timeinfo.tm_year < (2025 - 1900) && retry_count < max_retries)
  {
    vTaskDelay(pdMS_TO_TICKS(500));
    time(&now);
    localtime_r(&now, &timeinfo);
    retry_count++;
  }

  if (timeinfo.tm_year >= (2025 - 1900))
  {
    char time_str[64];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S %Z", &timeinfo);
    ESP_LOGI(TAG, "Time synchronized: %s", time_str);
  }
  else
  {
    ESP_LOGW(TAG, "NTP sync timed out, time may be inaccurate");
  }
}

// Initialize WiFi in station mode
esp_err_t wifi_init(void)
{
  ESP_LOGI(TAG, "Initializing WiFi...");

  // Create WiFi event group
  wifi_event_group = xEventGroupCreate();
  if (wifi_event_group == NULL)
  {
    ESP_LOGE(TAG, "Failed to create WiFi event group");
    return ESP_FAIL;
  }

  // Initialize NVS
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
  {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  // Initialize TCP/IP stack
  ESP_ERROR_CHECK(esp_netif_init());

  // Create default event loop
  ESP_ERROR_CHECK(esp_event_loop_create_default());

  // Create default WiFi station
  esp_netif_create_default_wifi_sta();

  // Initialize WiFi with default config
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  // Register event handlers
  ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                      ESP_EVENT_ANY_ID,
                                                      &wifi_event_handler,
                                                      NULL,
                                                      NULL));
  ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                      IP_EVENT_STA_GOT_IP,
                                                      &wifi_event_handler,
                                                      NULL,
                                                      NULL));

  // Set WiFi mode to station
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

  // Disable WiFi power save for better HTTP response latency
  // Power save can add 100-300ms delay per packet, which causes HTTP timeouts
  ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));
  ESP_LOGI(TAG, "WiFi power save disabled for improved HTTP responsiveness");

  ESP_LOGI(TAG, "WiFi initialization complete");
  return ESP_OK;
}

// Configure and start WiFi connection
esp_err_t wifi_connect(void)
{
  ESP_LOGI(TAG, "Configuring WiFi connection...");

  // Configure WiFi connection
  wifi_config_t wifi_config = {
      .sta = {
          .ssid = WIFI_SSID,
          .password = WIFI_PASSWORD,
          .threshold.authmode = WIFI_AUTH_WPA2_PSK,
      },
  };

  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));

  // Start WiFi
  ESP_ERROR_CHECK(esp_wifi_start());

  ESP_LOGI(TAG, "WiFi connection configured and started");
  return ESP_OK;
}

// Wait for WiFi connection to be established
esp_err_t wifi_wait_for_connection(void)
{
  ESP_LOGI(TAG, "Waiting for WiFi connection...");

  // Wait for connection (you might want to add a timeout here)
  EventBits_t bits = xEventGroupWaitBits(wifi_event_group,
                                         WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                         pdFALSE,
                                         pdFALSE,
                                         portMAX_DELAY);

  if (bits & WIFI_CONNECTED_BIT)
  {
    ESP_LOGI(TAG, "Connected to WiFi successfully!");
    return ESP_OK;
  }
  else if (bits & WIFI_FAIL_BIT)
  {
    ESP_LOGI(TAG, "Failed to connect to WiFi");
    return ESP_FAIL;
  }

  return ESP_FAIL;
}

// Get current IP address as string
esp_err_t wifi_get_ip_address(char *ip_buffer, size_t buffer_size)
{
  if (ip_buffer == NULL || buffer_size < 16)
  {
    return ESP_ERR_INVALID_ARG;
  }

  // Get default netif
  esp_netif_t *netif = esp_netif_get_default_netif();
  if (netif == NULL)
  {
    // Not connected, return empty string
    ip_buffer[0] = '\0';
    return ESP_OK;
  }

  // Get IP info
  esp_netif_ip_info_t ip_info;
  esp_err_t ret = esp_netif_get_ip_info(netif, &ip_info);
  if (ret != ESP_OK)
  {
    ip_buffer[0] = '\0';
    return ret;
  }

  // Format IP address
  if (snprintf(ip_buffer, buffer_size, IPSTR, IP2STR(&ip_info.ip)) >= (int)buffer_size)
  {
    // Buffer too small
    ip_buffer[0] = '\0';
    return ESP_ERR_INVALID_SIZE;
  }

  return ESP_OK;
}
