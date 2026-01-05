#include "web_server.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_littlefs.h"

static const char *TAG = "web_server";

httpd_handle_t server = NULL;

esp_err_t web_server_init(void)
{
  ESP_LOGI(TAG, "Initializing littleFS");

  esp_vfs_littlefs_conf_t conf = {
      .base_path = "/littlefs",
      .partition_label = NULL,
      .format_if_mount_failed = true};

  esp_err_t ret = esp_vfs_littlefs_register(&conf);
  if (ret != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to mount littleFS: %s", esp_err_to_name(ret));
    return ret;
  }

  size_t total = 0, used = 0;
  ret = esp_littlefs_info(NULL, &total, &used);
  if (ret != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to get littleFS info: %s", esp_err_to_name(ret));
  }
  else
  {
    ESP_LOGI(TAG, "littleFS: total=%zu, used=%zu", total, used);
  }

  // Initialize WebSocket client registry
  ret = ws_clients_init();
  if (ret != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to initialize WebSocket client registry");
    return ret;
  }

  ESP_LOGI(TAG, "littleFS initialization completed successfully");
  return ESP_OK;
}

esp_err_t web_server_start(void)
{
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.server_port = 3000;
  // Reduce resource usage to avoid conflicts
  config.max_open_sockets = 4; // Reduced from default 7
  config.max_uri_handlers = 8; // Reduced from default 8
  config.backlog_conn = 5;     // Reduced from default 5
  config.stack_size = 4096;    // Explicit stack size
  // Use custom URI matching function for proper wildcard support
  config.uri_match_fn = custom_uri_match;

  ESP_LOGI(TAG, "Starting HTTP server on port %d", config.server_port);
  esp_err_t ret = httpd_start(&server, &config);
  if (ret != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to start HTTP server: %s (%d)", esp_err_to_name(ret), ret);
    return ret;
  }

  // Register URI handlers - API handlers first, then static file handlers
  httpd_uri_t uri_version = {
      .uri = "/api/version",
      .method = HTTP_GET,
      .handler = version_handler,
      .user_ctx = NULL};
  httpd_register_uri_handler(server, &uri_version);

  httpd_uri_t uri_sensor = {
      .uri = "/ws/sensor-data",
      .method = HTTP_GET,
      .handler = sensor_data_handler,
      .user_ctx = NULL,
      .is_websocket = true};
  httpd_register_uri_handler(server, &uri_sensor);

  // Single catch-all handler for static files (like ESP-IDF file serving example)
  httpd_uri_t uri_static = {
      .uri = "/*",
      .method = HTTP_GET,
      .handler = static_file_handler,
      .user_ctx = NULL};
  httpd_register_uri_handler(server, &uri_static);

  ESP_LOGI(TAG, "Static file handlers registered");

  ESP_LOGI(TAG, "HTTP server started");
  return ESP_OK;
}
