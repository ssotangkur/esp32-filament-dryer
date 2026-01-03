#include "web_server.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_littlefs.h"
#include "version.h"
#include "circular_buffer.h"
#include "temp.h"
#include <string.h>

static const char *TAG = "web_server";

static httpd_handle_t server = NULL;

// Custom URI matching function for proper wildcard support
static bool custom_uri_match(const char *reference_uri, const char *uri_to_match, size_t match_upto)
{
  // Handle exact matches
  if (strchr(reference_uri, '*') == NULL)
  {
    return strcmp(reference_uri, uri_to_match) == 0;
  }

  // Handle wildcard patterns (ending with *)
  size_t ref_len = strlen(reference_uri);
  if (reference_uri[ref_len - 1] == '*')
  {
    size_t prefix_len = ref_len - 1;
    return strncmp(reference_uri, uri_to_match, prefix_len) == 0;
  }

  // Fallback to exact match
  return strcmp(reference_uri, uri_to_match) == 0;
}

// Helper function to get MIME type from file extension
static const char *get_mime_type(const char *filepath)
{
  const char *ext = strrchr(filepath, '.');
  if (ext)
  {
    if (strcmp(ext, ".html") == 0 || strcmp(ext, ".htm") == 0)
      return "text/html";
    if (strcmp(ext, ".css") == 0)
      return "text/css";
    if (strcmp(ext, ".js") == 0)
      return "application/javascript";
    if (strcmp(ext, ".json") == 0)
      return "application/json";
    if (strcmp(ext, ".png") == 0)
      return "image/png";
    if (strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0)
      return "image/jpeg";
    if (strcmp(ext, ".gif") == 0)
      return "image/gif";
    if (strcmp(ext, ".svg") == 0)
      return "image/svg+xml";
    if (strcmp(ext, ".ico") == 0)
      return "image/x-icon";
    if (strcmp(ext, ".txt") == 0)
      return "text/plain";
  }
  return "application/octet-stream"; // Default
}

// Handler for static files
static esp_err_t static_file_handler(httpd_req_t *req)
{
  ESP_LOGI(TAG, "Static file request: %s", req->uri);

  char filepath[256];

  // Default to index.html for root
  if (strcmp(req->uri, "/") == 0 || strcmp(req->uri, "/index.html") == 0)
  {
    strcpy(filepath, "/littlefs/index.html");
  }
  else
  {
    // Safely build the file path
    strcpy(filepath, "/littlefs");
    strncat(filepath, req->uri, sizeof(filepath) - strlen("/littlefs") - 1);
  }

  ESP_LOGI(TAG, "Looking for file: %s", filepath);

  FILE *file = fopen(filepath, "r");
  if (file == NULL)
  {
    ESP_LOGE(TAG, "File not found: %s", filepath);
    httpd_resp_send_404(req);
    return ESP_OK;
  }

  ESP_LOGI(TAG, "File found, serving: %s", filepath);

  // Set appropriate MIME type based on file extension
  const char *mime_type = get_mime_type(filepath);
  httpd_resp_set_type(req, mime_type);

  char buffer[1024];
  size_t read_len;

  while ((read_len = fread(buffer, 1, sizeof(buffer), file)) > 0)
  {
    httpd_resp_send_chunk(req, buffer, read_len);
  }

  fclose(file);
  httpd_resp_send_chunk(req, NULL, 0);
  return ESP_OK;
}

// Handler for /api/version
static esp_err_t version_handler(httpd_req_t *req)
{
  httpd_resp_set_type(req, "text/plain");
  httpd_resp_sendstr(req, FIRMWARE_VERSION_STRING);
  return ESP_OK;
}

// Sensor data handler - returns JSON with temperature data
static esp_err_t sensor_data_handler(httpd_req_t *req)
{
  // Send current temperature data from both sensors
  circular_buffer_t *air_buffer = get_air_temp_buffer();
  circular_buffer_t *heater_buffer = get_heater_temp_buffer();

  char json_data[1024];
  strcpy(json_data, "[");

  int count = 0;
  // Send from air buffer
  if (air_buffer)
  {
    size_t air_count = circular_buffer_count(air_buffer);
    for (size_t i = 0; i < air_count; i++)
    {
      if (count > 0)
        strcat(json_data, ",");
      temp_sample_t sample;
      if (circular_buffer_get_at_index(air_buffer, i, &sample))
      {
        char item[128];
        snprintf(item, sizeof(item), "{\"sensor\":\"air\",\"temperature\":%.2f,\"timestamp\":%lu}", sample.temperature, (unsigned long)sample.timestamp);
        strcat(json_data, item);
        count++;
      }
    }
  }

  // Send from heater buffer
  if (heater_buffer)
  {
    size_t heater_count = circular_buffer_count(heater_buffer);
    for (size_t i = 0; i < heater_count; i++)
    {
      if (count > 0)
        strcat(json_data, ",");
      temp_sample_t sample;
      if (circular_buffer_get_at_index(heater_buffer, i, &sample))
      {
        char item[128];
        snprintf(item, sizeof(item), "{\"sensor\":\"heater\",\"temperature\":%.2f,\"timestamp\":%lu}", sample.temperature, (unsigned long)sample.timestamp);
        strcat(json_data, item);
        count++;
      }
    }
  }

  strcat(json_data, "]");

  httpd_resp_set_type(req, "application/json");
  httpd_resp_sendstr(req, json_data);

  return ESP_OK;
}

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
      .uri = "/api/sensor-data",
      .method = HTTP_GET,
      .handler = sensor_data_handler,
      .user_ctx = NULL};
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
