#include "ota.h"
#include "wifi_credentials.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "esp_lvgl_port.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "sysmon_wrapper.h"
#include <string.h>

static const char *TAG = "OTA";

static bool ota_in_progress = false;
static int ota_progress = -1;
static SemaphoreHandle_t ota_mutex;

static esp_err_t ota_http_event_handler(esp_http_client_event_t *evt)
{
  switch (evt->event_id)
  {
  case HTTP_EVENT_ERROR:
    ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
    break;
  case HTTP_EVENT_ON_CONNECTED:
    ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
    break;
  case HTTP_EVENT_HEADER_SENT:
    ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
    break;
  case HTTP_EVENT_ON_HEADER:
    ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
    break;
  case HTTP_EVENT_ON_DATA:
    ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
    break;
  case HTTP_EVENT_ON_FINISH:
    ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
    break;
  case HTTP_EVENT_DISCONNECTED:
    ESP_LOGD(TAG, "HTTP_EVENT_DISCONNECTED");
    break;
  case HTTP_EVENT_REDIRECT:
    ESP_LOGD(TAG, "HTTP_EVENT_REDIRECT");
    break;
  }
  return ESP_OK;
}

static void ota_task(void *pvParameter)
{
  char *url = (char *)pvParameter;

  ESP_LOGI(TAG, "Starting OTA update from: %s", url);

  // Stop LVGL to prevent display crashes during OTA
  ESP_LOGI(TAG, "Stopping LVGL during OTA update");
  lvgl_port_stop();

  xSemaphoreTake(ota_mutex, portMAX_DELAY);
  ota_in_progress = true;
  ota_progress = 0;
  xSemaphoreGive(ota_mutex);

  esp_http_client_config_t config = {
      .url = url,
      .event_handler = ota_http_event_handler,
      .keep_alive_enable = true,
  };

  esp_https_ota_config_t ota_config = {
      .http_config = &config,
  };

  esp_err_t ret = esp_https_ota(&ota_config);

  xSemaphoreTake(ota_mutex, portMAX_DELAY);
  ota_in_progress = false;
  ota_progress = -1;
  xSemaphoreGive(ota_mutex);

  if (ret == ESP_OK)
  {
    ESP_LOGI(TAG, "OTA update successful, restarting...");
    esp_restart();
  }
  else
  {
    ESP_LOGE(TAG, "OTA update failed: %s", esp_err_to_name(ret));
    lvgl_port_resume(); // Resume LVGL on error
  }

  vTaskDelete(NULL);
}

static void ota_https_task(void *pvParameter)
{
  char *url = (char *)pvParameter;

  ESP_LOGI(TAG, "Starting HTTPS OTA update from: %s", url);

  // Stop LVGL to prevent display crashes during OTA
  ESP_LOGI(TAG, "Stopping LVGL during OTA update");
  lvgl_port_stop();

  xSemaphoreTake(ota_mutex, portMAX_DELAY);
  ota_in_progress = true;
  ota_progress = 0;
  xSemaphoreGive(ota_mutex);

  esp_http_client_config_t config = {
      .url = url,
      .event_handler = ota_http_event_handler,
      .keep_alive_enable = true,
  };

  esp_https_ota_config_t ota_config = {
      .http_config = &config,
  };

  esp_err_t ret = esp_https_ota(&ota_config);

  xSemaphoreTake(ota_mutex, portMAX_DELAY);
  ota_in_progress = false;
  ota_progress = -1;
  xSemaphoreGive(ota_mutex);

  if (ret == ESP_OK)
  {
    ESP_LOGI(TAG, "OTA update successful, restarting...");
    esp_restart();
  }
  else
  {
    ESP_LOGE(TAG, "OTA update failed: %s", esp_err_to_name(ret));
    lvgl_port_resume(); // Resume LVGL on error
  }

  vTaskDelete(NULL);
}

static void ota_http_task(void *pvParameter)
{
  char *url = (char *)pvParameter;

  ESP_LOGI(TAG, "Starting HTTP OTA update from: %s", url);

  // Stop LVGL to prevent display crashes during OTA
  ESP_LOGI(TAG, "Stopping LVGL during OTA update");
  lvgl_port_stop();

  xSemaphoreTake(ota_mutex, portMAX_DELAY);
  ota_in_progress = true;
  ota_progress = 0;
  xSemaphoreGive(ota_mutex);

  esp_http_client_config_t config = {
      .url = url,
      .event_handler = ota_http_event_handler,
  };

  esp_http_client_handle_t client = esp_http_client_init(&config);
  if (client == NULL)
  {
    ESP_LOGE(TAG, "Failed to initialize HTTP client");
    lvgl_port_resume(); // Resume LVGL on error
    xSemaphoreTake(ota_mutex, portMAX_DELAY);
    ota_in_progress = false;
    ota_progress = -1;
    xSemaphoreGive(ota_mutex);
    vTaskDelete(NULL);
    return;
  }

  esp_err_t err = esp_http_client_open(client, 0);
  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
    esp_http_client_cleanup(client);
    lvgl_port_resume(); // Resume LVGL on error
    xSemaphoreTake(ota_mutex, portMAX_DELAY);
    ota_in_progress = false;
    ota_progress = -1;
    xSemaphoreGive(ota_mutex);
    vTaskDelete(NULL);
    return;
  }

  int content_length = esp_http_client_fetch_headers(client);
  if (content_length < 0)
  {
    ESP_LOGE(TAG, "Failed to get content length");
    esp_http_client_cleanup(client);
    lvgl_port_resume(); // Resume LVGL on error
    xSemaphoreTake(ota_mutex, portMAX_DELAY);
    ota_in_progress = false;
    ota_progress = -1;
    xSemaphoreGive(ota_mutex);
    vTaskDelete(NULL);
    return;
  }

  esp_ota_handle_t ota_handle;
  const esp_partition_t *update_partition = esp_ota_get_next_update_partition(NULL);
  err = esp_ota_begin(update_partition, content_length, &ota_handle);
  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "esp_ota_begin failed: %s", esp_err_to_name(err));
    esp_http_client_cleanup(client);
    lvgl_port_resume(); // Resume LVGL on error
    xSemaphoreTake(ota_mutex, portMAX_DELAY);
    ota_in_progress = false;
    ota_progress = -1;
    xSemaphoreGive(ota_mutex);
    vTaskDelete(NULL);
    return;
  }

  ESP_LOGI(TAG, "Writing to partition subtype %d at offset 0x%x",
           update_partition->subtype, update_partition->address);

  int total_read_len = 0;
  int read_len;
  char *buffer = malloc(4096);
  if (buffer == NULL)
  {
    ESP_LOGE(TAG, "Failed to allocate buffer");
    esp_ota_abort(ota_handle);
    esp_http_client_cleanup(client);
    lvgl_port_resume(); // Resume LVGL on error
    xSemaphoreTake(ota_mutex, portMAX_DELAY);
    ota_in_progress = false;
    ota_progress = -1;
    xSemaphoreGive(ota_mutex);
    vTaskDelete(NULL);
    return;
  }

  while (1)
  {
    read_len = esp_http_client_read(client, buffer, 4096);
    if (read_len < 0)
    {
      ESP_LOGE(TAG, "Error reading data");
      break;
    }
    if (read_len == 0)
    {
      ESP_LOGI(TAG, "Connection closed");
      break;
    }

    err = esp_ota_write(ota_handle, buffer, read_len);
    if (err != ESP_OK)
    {
      ESP_LOGE(TAG, "esp_ota_write failed: %s", esp_err_to_name(err));
      break;
    }

    total_read_len += read_len;

    // Update progress
    if (content_length > 0)
    {
      int progress = (total_read_len * 100) / content_length;
      xSemaphoreTake(ota_mutex, portMAX_DELAY);
      ota_progress = progress;
      xSemaphoreGive(ota_mutex);
    }

    ESP_LOGD(TAG, "Written %d/%d bytes", total_read_len, content_length);
  }

  free(buffer);

  esp_http_client_cleanup(client);

  if (err == ESP_OK && read_len == 0)
  {
    err = esp_ota_end(ota_handle);
    if (err == ESP_OK)
    {
      err = esp_ota_set_boot_partition(update_partition);
      if (err == ESP_OK)
      {
        ESP_LOGI(TAG, "OTA update successful, performing clean restart...");
        xSemaphoreTake(ota_mutex, portMAX_DELAY);
        ota_in_progress = false;
        ota_progress = -1;
        xSemaphoreGive(ota_mutex);

        // Force immediate watchdog reset for clean system state
        // This ensures no stale resources or LVGL state issues after OTA
        ESP_LOGI(TAG, "Triggering watchdog reset...");
        esp_system_abort("OTA completed successfully - clean restart");
      }
      else
      {
        ESP_LOGE(TAG, "esp_ota_set_boot_partition failed: %s", esp_err_to_name(err));
      }
    }
    else
    {
      ESP_LOGE(TAG, "esp_ota_end failed: %s", esp_err_to_name(err));
    }
  }
  else
  {
    esp_ota_abort(ota_handle);
    ESP_LOGE(TAG, "OTA update failed");
    lvgl_port_resume(); // Resume LVGL on failure
  }

  xSemaphoreTake(ota_mutex, portMAX_DELAY);
  ota_in_progress = false;
  ota_progress = -1;
  xSemaphoreGive(ota_mutex);

  vTaskDelete(NULL);
}

esp_err_t ota_init(void)
{
  ota_mutex = xSemaphoreCreateMutex();
  if (ota_mutex == NULL)
  {
    ESP_LOGE(TAG, "Failed to create OTA mutex");
    return ESP_ERR_NO_MEM;
  }

  // Mark current app as valid to prevent rollback on next boot
  esp_ota_mark_app_valid_cancel_rollback();

  ESP_LOGI(TAG, "OTA initialized");
  return ESP_OK;
}

esp_err_t ota_update_from_https_url(const char *url)
{
  if (ota_in_progress)
  {
    ESP_LOGW(TAG, "OTA update already in progress");
    return ESP_ERR_INVALID_STATE;
  }

  // Create HTTPS OTA task with sysmon monitoring
  if (sysmon_xTaskCreate(&ota_https_task, "ota_https_task", 8192, (void *)url, 5, NULL) != pdPASS)
  {
    ESP_LOGE(TAG, "Failed to create OTA task");
    return ESP_ERR_NO_MEM;
  }

  return ESP_OK;
}

esp_err_t ota_update_from_http_url(const char *url)
{
  if (ota_in_progress)
  {
    ESP_LOGW(TAG, "OTA update already in progress");
    return ESP_ERR_INVALID_STATE;
  }

  // Create HTTP OTA task with sysmon monitoring
  if (sysmon_xTaskCreate(&ota_http_task, "ota_http_task", 8192, (void *)url, 5, NULL) != pdPASS)
  {
    ESP_LOGE(TAG, "Failed to create OTA task");
    return ESP_ERR_NO_MEM;
  }

  return ESP_OK;
}

esp_err_t ota_update_from_url(const char *url)
{
  if (ota_in_progress)
  {
    ESP_LOGW(TAG, "OTA update already in progress");
    return ESP_ERR_INVALID_STATE;
  }

  // Auto-detect HTTP vs HTTPS
  if (strncmp(url, "https://", 8) == 0)
  {
    return ota_update_from_https_url(url);
  }
  else if (strncmp(url, "http://", 7) == 0)
  {
    return ota_update_from_http_url(url);
  }
  else
  {
    ESP_LOGE(TAG, "Invalid URL protocol. Must be http:// or https://");
    return ESP_ERR_INVALID_ARG;
  }
}

bool ota_is_updating(void)
{
  bool updating;
  xSemaphoreTake(ota_mutex, portMAX_DELAY);
  updating = ota_in_progress;
  xSemaphoreGive(ota_mutex);
  return updating;
}

bool ota_check_for_update(const char *ota_base_url)
{
  if (ota_in_progress)
  {
    ESP_LOGW(TAG, "OTA update already in progress");
    return false;
  }

  // Construct the version URL
  char version_url[256];
  if (strstr(ota_base_url, "/firmware.bin") != NULL)
  {
    // Remove /firmware.bin from the end and add /version
    size_t base_len = strlen(ota_base_url) - strlen("/firmware.bin");
    strncpy(version_url, ota_base_url, base_len);
    version_url[base_len] = '\0';
    strcat(version_url, "/version");
  }
  else
  {
    // Assume it's a base URL, add /version
    strcpy(version_url, ota_base_url);
    if (version_url[strlen(version_url) - 1] != '/')
    {
      strcat(version_url, "/");
    }
    strcat(version_url, "version");
  }

  ESP_LOGI(TAG, "Checking for updates at: %s", version_url);

  esp_http_client_config_t config = {
      .url = version_url,
      .method = HTTP_METHOD_GET,
      .timeout_ms = 5000, // 5 second timeout
  };

  esp_http_client_handle_t client = esp_http_client_init(&config);
  if (client == NULL)
  {
    ESP_LOGE(TAG, "Failed to initialize HTTP client for version check");
    return false;
  }

  esp_err_t err = esp_http_client_open(client, 0);
  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to open HTTP connection for version check: %s", esp_err_to_name(err));
    esp_http_client_cleanup(client);
    return false;
  }

  int content_length = esp_http_client_fetch_headers(client);
  if (content_length < 0 || content_length > 1024)
  {
    ESP_LOGE(TAG, "Invalid content length for version check: %d", content_length);
    esp_http_client_cleanup(client);
    return false;
  }

  // Read the version response
  char *buffer = malloc(content_length + 1);
  if (buffer == NULL)
  {
    ESP_LOGE(TAG, "Failed to allocate buffer for version check");
    esp_http_client_cleanup(client);
    return false;
  }

  int read_len = esp_http_client_read(client, buffer, content_length);
  esp_http_client_cleanup(client);

  if (read_len <= 0)
  {
    ESP_LOGE(TAG, "Failed to read version response");
    free(buffer);
    return false;
  }

  buffer[read_len] = '\0';

  // Parse JSON response (simple parsing for "version": "X.X.X")
  char *version_start = strstr(buffer, "\"version\"");
  if (version_start == NULL)
  {
    ESP_LOGE(TAG, "Version field not found in response");
    free(buffer);
    return false;
  }

  // Find the version value
  version_start = strstr(version_start, ":");
  if (version_start == NULL)
  {
    ESP_LOGE(TAG, "Version value not found in response");
    free(buffer);
    return false;
  }

  // Skip to the actual version string
  version_start += 2; // Skip ": "
  char *version_end = strstr(version_start, "\"");
  if (version_end == NULL)
  {
    ESP_LOGE(TAG, "Version value end not found");
    free(buffer);
    return false;
  }

  *version_end = '\0'; // Null terminate the version string

  ESP_LOGI(TAG, "Current firmware version: %s", FIRMWARE_VERSION);
  ESP_LOGI(TAG, "Available firmware version: %s", version_start);

  // Compare versions using semantic versioning
  bool update_available = is_version_newer(FIRMWARE_VERSION, version_start);

  if (update_available)
  {
    ESP_LOGI(TAG, "Firmware update available!");
  }
  else
  {
    ESP_LOGI(TAG, "Firmware is up to date");
  }

  free(buffer);
  return update_available;
}

int ota_get_progress(void)
{
  int progress;
  xSemaphoreTake(ota_mutex, portMAX_DELAY);
  progress = ota_progress;
  xSemaphoreGive(ota_mutex);
  return progress;
}

// Periodic OTA checking task
static void ota_check_task(void *pvParameter)
{
  ESP_LOGI(TAG, "OTA check task started - checking for updates every 5 seconds");

  while (1)
  {
    // Check for OTA updates
    if (ota_check_for_update(OTA_URL))
    {
      ESP_LOGI(TAG, "New firmware version available! Starting update...");
      ota_update_from_url(OTA_URL);
    }
    else
    {
      ESP_LOGD(TAG, "Firmware is up to date");
    }

    // Wait 5 seconds before next check
    vTaskDelay(pdMS_TO_TICKS(30000));
  }
}

esp_err_t ota_start_auto_check(void)
{
  // Start periodic OTA checking task (checks every 30 seconds)
  // Use sysmon wrapper to enable stack monitoring
  if (sysmon_xTaskCreate(&ota_check_task, "ota_check_task", 4096, NULL, 5, NULL) != pdPASS)
  {
    ESP_LOGE(TAG, "Failed to create OTA check task");
    return ESP_ERR_NO_MEM;
  }

  ESP_LOGI(TAG, "Automatic OTA checking started");
  return ESP_OK;
}
