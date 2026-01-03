#include "web_server.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_littlefs.h"
#include "version.h"
#include "circular_buffer.h"
#include "temp.h"
#include <string.h>
#include <stdlib.h>

static const char *TAG = "web_server";

static httpd_handle_t server = NULL;

// WebSocket client registry for sensor data subscriptions
#define MAX_WS_CLIENTS 8
static int ws_client_fds[MAX_WS_CLIENTS];
static SemaphoreHandle_t ws_clients_mutex = NULL;
static int ws_client_count = 0;

// Initialize WebSocket client registry
static esp_err_t ws_clients_init(void)
{
  ws_clients_mutex = xSemaphoreCreateMutex();
  if (ws_clients_mutex == NULL)
  {
    ESP_LOGE(TAG, "Failed to create WebSocket clients mutex");
    return ESP_ERR_NO_MEM;
  }

  // Initialize client array
  memset(ws_client_fds, 0, sizeof(ws_client_fds));
  ws_client_count = 0;

  ESP_LOGI(TAG, "WebSocket client registry initialized");
  return ESP_OK;
}

// Add a WebSocket client to the registry
static esp_err_t ws_client_add(int sockfd)
{
  if (xSemaphoreTake(ws_clients_mutex, pdMS_TO_TICKS(1000)) != pdTRUE)
  {
    ESP_LOGE(TAG, "Failed to take WebSocket clients mutex");
    return ESP_ERR_TIMEOUT;
  }

  // Check if already registered
  for (int i = 0; i < ws_client_count; i++)
  {
    if (ws_client_fds[i] == sockfd)
    {
      xSemaphoreGive(ws_clients_mutex);
      return ESP_OK; // Already registered
    }
  }

  // Add new client if space available
  if (ws_client_count < MAX_WS_CLIENTS)
  {
    ws_client_fds[ws_client_count] = sockfd;
    ws_client_count++;
    ESP_LOGI(TAG, "WebSocket client added: fd=%d, total clients=%d", sockfd, ws_client_count);
    xSemaphoreGive(ws_clients_mutex);
    return ESP_OK;
  }
  else
  {
    ESP_LOGW(TAG, "WebSocket client registry full, cannot add fd=%d", sockfd);
    xSemaphoreGive(ws_clients_mutex);
    return ESP_ERR_NO_MEM;
  }
}

// Remove a WebSocket client from the registry
static void ws_client_remove(int sockfd)
{
  if (xSemaphoreTake(ws_clients_mutex, pdMS_TO_TICKS(1000)) != pdTRUE)
  {
    ESP_LOGE(TAG, "Failed to take WebSocket clients mutex for removal");
    return;
  }

  for (int i = 0; i < ws_client_count; i++)
  {
    if (ws_client_fds[i] == sockfd)
    {
      // Shift remaining clients
      for (int j = i; j < ws_client_count - 1; j++)
      {
        ws_client_fds[j] = ws_client_fds[j + 1];
      }
      ws_client_count--;
      ws_client_fds[ws_client_count] = 0; // Clear last slot
      ESP_LOGI(TAG, "WebSocket client removed: fd=%d, remaining clients=%d", sockfd, ws_client_count);
      break;
    }
  }

  xSemaphoreGive(ws_clients_mutex);
}

// Broadcast sensor data to all subscribed WebSocket clients
static void ws_broadcast_sensor_data(const char *json_data)
{
  if (xSemaphoreTake(ws_clients_mutex, pdMS_TO_TICKS(100)) != pdTRUE)
  {
    ESP_LOGE(TAG, "Failed to take WebSocket clients mutex for broadcast");
    return;
  }

  httpd_ws_frame_t ws_resp = {
      .final = true,
      .fragmented = false,
      .type = HTTPD_WS_TYPE_TEXT,
      .payload = (uint8_t *)json_data,
      .len = strlen(json_data)};

  // Send to all clients
  for (int i = 0; i < ws_client_count; i++)
  {
    int sockfd = ws_client_fds[i];
    esp_err_t ret = httpd_ws_send_frame_async(server, sockfd, &ws_resp);
    if (ret != ESP_OK)
    {
      ESP_LOGW(TAG, "Failed to send data to WebSocket client fd=%d: %s", sockfd, esp_err_to_name(ret));
      // Client might be disconnected, will be cleaned up on next attempt
    }
  }

  ESP_LOGD(TAG, "Broadcasted sensor data to %d clients", ws_client_count);
  xSemaphoreGive(ws_clients_mutex);
}

// Send initial sensor data to a specific WebSocket client
static void ws_broadcast_initial_data(int sockfd)
{
  // Get sensor handles
  temp_sensor_handle_t air_sensor = temp_sensor_get_air_sensor();
  temp_sensor_handle_t heater_sensor = temp_sensor_get_heater_sensor();

  char json_data[1024];
  strcpy(json_data, "[");

  int count = 0;

  // Send all samples from air sensor
  if (air_sensor)
  {
    size_t air_count = temp_sensor_get_sample_count(air_sensor);
    for (size_t i = 0; i < air_count; i++)
    {
      if (count > 0)
        strcat(json_data, ",");
      temp_sample_t sample;
      if (temp_sensor_get_sample(air_sensor, i, &sample))
      {
        char item[128];
        snprintf(item, sizeof(item), "{\"sensor\":\"air\",\"temperature\":%.2f,\"timestamp\":%lu}", sample.temperature, (unsigned long)sample.timestamp);
        strcat(json_data, item);
        count++;
      }
    }
  }

  // Send all samples from heater sensor
  if (heater_sensor)
  {
    size_t heater_count = temp_sensor_get_sample_count(heater_sensor);
    for (size_t i = 0; i < heater_count; i++)
    {
      if (count > 0)
        strcat(json_data, ",");
      temp_sample_t sample;
      if (temp_sensor_get_sample(heater_sensor, i, &sample))
      {
        char item[128];
        snprintf(item, sizeof(item), "{\"sensor\":\"heater\",\"temperature\":%.2f,\"timestamp\":%lu}", sample.temperature, (unsigned long)sample.timestamp);
        strcat(json_data, item);
        count++;
      }
    }
  }

  strcat(json_data, "]");

  httpd_ws_frame_t ws_resp = {
      .final = true,
      .fragmented = false,
      .type = HTTPD_WS_TYPE_TEXT,
      .payload = (uint8_t *)json_data,
      .len = strlen(json_data)};

  esp_err_t ret = httpd_ws_send_frame_async(server, sockfd, &ws_resp);
  if (ret != ESP_OK)
  {
    ESP_LOGW(TAG, "Failed to send initial data to WebSocket client fd=%d: %s", sockfd, esp_err_to_name(ret));
  }
  else
  {
    ESP_LOGI(TAG, "Sent initial sensor data to WebSocket client fd=%d (%d samples)", sockfd, count);
  }
}

// Broadcast latest sensor readings to all subscribed WebSocket clients
void ws_broadcast_latest_sensor_data(void)
{
  // Get sensor handles
  temp_sensor_handle_t air_sensor = temp_sensor_get_air_sensor();
  temp_sensor_handle_t heater_sensor = temp_sensor_get_heater_sensor();

  char json_data[256];
  strcpy(json_data, "[");

  int count = 0;

  // Send latest air sensor reading
  if (air_sensor)
  {
    temp_sample_t sample;
    if (temp_sensor_get_latest_sample(air_sensor, &sample))
    {
      char item[128];
      snprintf(item, sizeof(item), "{\"sensor\":\"air\",\"temperature\":%.2f,\"timestamp\":%lu}", sample.temperature, (unsigned long)sample.timestamp);
      strcat(json_data, item);
      count++;
    }
  }

  // Send latest heater sensor reading
  if (heater_sensor)
  {
    if (count > 0)
      strcat(json_data, ",");
    temp_sample_t sample;
    if (temp_sensor_get_latest_sample(heater_sensor, &sample))
    {
      char item[128];
      snprintf(item, sizeof(item), "{\"sensor\":\"heater\",\"temperature\":%.2f,\"timestamp\":%lu}", sample.temperature, (unsigned long)sample.timestamp);
      strcat(json_data, item);
      count++;
    }
  }

  strcat(json_data, "]");

  if (count > 0)
  {
    ws_broadcast_sensor_data(json_data);
  }
}

// WebSocket session context for handling fragmented messages
struct ws_session_ctx
{
  char *buffer;
  size_t len;
  bool accumulating_text;
};

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

// Sensor data handler - WebSocket handler for temperature data with subscription
static esp_err_t sensor_data_handler(httpd_req_t *req)
{
  // Handle WebSocket handshake completion (new connection)
  if (req->method == HTTP_GET)
  {
    int sockfd = httpd_req_to_sockfd(req);
    ESP_LOGI(TAG, "WebSocket handshake completed for fd=%d", sockfd);

    // Subscribe client to sensor data updates
    ws_client_add(sockfd);

    return ESP_OK;
  }

  // Get or create session context
  struct ws_session_ctx *sess = (struct ws_session_ctx *)req->sess_ctx;
  if (sess == NULL)
  {
    sess = calloc(1, sizeof(struct ws_session_ctx));
    if (sess == NULL)
    {
      ESP_LOGE(TAG, "Failed to allocate session context");
      return ESP_ERR_NO_MEM;
    }
    req->sess_ctx = sess;
  }

  httpd_ws_frame_t ws_pkt;
  memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));

  // Try to receive a frame
  esp_err_t ret = httpd_ws_recv_frame(req, &ws_pkt, 0);
  if (ret != ESP_OK)
  {
    // Connection likely closed - unsubscribe client
    int sockfd = httpd_req_to_sockfd(req);
    ESP_LOGI(TAG, "WebSocket connection closed for fd=%d", sockfd);
    ws_client_remove(sockfd);

    // Clean up session context
    if (sess->buffer)
    {
      free(sess->buffer);
    }
    free(sess);
    req->sess_ctx = NULL;

    return ESP_OK;
  }

  ESP_LOGI(TAG, "WS frame type: %d, len: %d, final: %d", ws_pkt.type, ws_pkt.len, ws_pkt.final);

  // Handle frame based on type (for future extensibility)
  if (ws_pkt.type == HTTPD_WS_TYPE_TEXT)
  {
    // Allocate buffer for payload
    ws_pkt.payload = malloc(ws_pkt.len + 1);
    if (ws_pkt.payload == NULL)
    {
      ESP_LOGE(TAG, "Failed to allocate memory for WS payload");
      return ESP_ERR_NO_MEM;
    }

    // Second call to get actual data
    ret = httpd_ws_recv_frame(req, &ws_pkt, ws_pkt.len);
    if (ret != ESP_OK)
    {
      ESP_LOGE(TAG, "httpd_ws_recv_frame failed to get data: %s", esp_err_to_name(ret));
      free(ws_pkt.payload);
      return ESP_OK; // Don't close connection
    }

    // Process text message
    ws_pkt.payload[ws_pkt.len] = '\0';
    ESP_LOGI(TAG, "Received text message from client: %s", (char *)ws_pkt.payload);

    // Check if client is requesting initial data
    if (ws_pkt.len == 8 && strcmp((char *)ws_pkt.payload, "get_data") == 0)
    {
      int sockfd = httpd_req_to_sockfd(req);
      ESP_LOGI(TAG, "Client requested initial data, sending buffered readings");
      ws_broadcast_initial_data(sockfd);
    }

    free(ws_pkt.payload);
  }
  else if (ws_pkt.type == HTTPD_WS_TYPE_CONTINUE && sess->accumulating_text)
  {
    // Handle fragmented messages if needed in the future
    // For now, subscription model doesn't expect fragmented messages from clients
    ESP_LOGW(TAG, "Unexpected fragmented message from client");
  }
  // Ignore other frame types

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
