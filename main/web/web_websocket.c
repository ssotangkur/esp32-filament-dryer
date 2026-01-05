#include "web_server.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "circular_buffer.h"
#include "temp.h"
#include <string.h>
#include <stdlib.h>

static const char *TAG = "web_server";

// WebSocket client registry for sensor data subscriptions
#define MAX_WS_CLIENTS 8
static int ws_client_fds[MAX_WS_CLIENTS];
static SemaphoreHandle_t ws_clients_mutex = NULL;
static int ws_client_count = 0;

// WebSocket session context for handling fragmented messages
struct ws_session_ctx
{
  char *buffer;
  size_t len;
  bool accumulating_text;
  int sockfd;              // Store the socket fd for reliable access
  bool handshake_complete; // Track if WebSocket handshake is fully complete
};

// Get or create session context for WebSocket connections
static esp_err_t get_or_create_session_context(httpd_req_t *req, struct ws_session_ctx **session_out)
{
  struct ws_session_ctx *sess = (struct ws_session_ctx *)req->sess_ctx;
  if (sess == NULL)
  {
    sess = calloc(1, sizeof(struct ws_session_ctx));
    if (sess == NULL)
    {
      ESP_LOGE(TAG, "Failed to allocate session context");
      *session_out = NULL;
      return ESP_ERR_NO_MEM;
    }
    req->sess_ctx = sess;

    // Store the socket fd in session context for reliable access
    sess->sockfd = httpd_req_to_sockfd(req);
  }

  *session_out = sess;
  return ESP_OK;
}

// Clean up WebSocket session context
static void cleanup_session_context(httpd_req_t *req, struct ws_session_ctx *sess)
{
  if (sess != NULL)
  {
    // Clean up session context
    if (sess->buffer)
    {
      free(sess->buffer);
    }
    free(sess);
    req->sess_ctx = NULL;
  }
}

// Initialize WebSocket client registry
esp_err_t ws_clients_init(void)
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

// Context structure for queued broadcast work
typedef struct
{
  char json_data[256]; // Copy of the data to broadcast
} broadcast_work_ctx_t;

// Work function to broadcast sensor data (executed in HTTP server context)
static void broadcast_work_function(void *arg)
{
  broadcast_work_ctx_t *ctx = (broadcast_work_ctx_t *)arg;
  if (ctx == NULL)
  {
    ESP_LOGE(TAG, "Broadcast work context is NULL");
    return;
  }

  if (xSemaphoreTake(ws_clients_mutex, pdMS_TO_TICKS(100)) != pdTRUE)
  {
    ESP_LOGE(TAG, "Failed to take WebSocket clients mutex for broadcast");
    free(ctx);
    return;
  }

  httpd_ws_frame_t ws_resp = {
      .final = true,
      .fragmented = false,
      .type = HTTPD_WS_TYPE_TEXT,
      .payload = (uint8_t *)ctx->json_data,
      .len = strlen(ctx->json_data)};

  // Send to all clients synchronously (since httpd_ws_send_frame_async is actually synchronous)
  for (int i = 0; i < ws_client_count; i++)
  {
    int sockfd = ws_client_fds[i];
    esp_err_t ret = httpd_ws_send_frame_async(server, sockfd, &ws_resp);
    if (ret != ESP_OK)
    {
      ESP_LOGW(TAG, "Failed to broadcast to WebSocket client fd=%d: %s", sockfd, esp_err_to_name(ret));
      // Immediately remove disconnected client
      ws_client_remove(sockfd);
      i--; // Adjust loop counter
    }
    else
    {
      ESP_LOGD(TAG, "Broadcasted sensor data to client fd=%d", sockfd);
    }
  }

  ESP_LOGD(TAG, "Broadcast work completed for %d clients", ws_client_count);
  xSemaphoreGive(ws_clients_mutex);
  free(ctx); // Safe to free now since httpd_ws_send_frame_async is synchronous
}

// Broadcast sensor data to all subscribed WebSocket clients using queued work
static void ws_broadcast_sensor_data(const char *json_data)
{
  // Check if server is initialized
  if (server == NULL)
  {
    ESP_LOGW(TAG, "Cannot broadcast - HTTP server not initialized");
    return;
  }

  // Allocate work context
  broadcast_work_ctx_t *ctx = malloc(sizeof(broadcast_work_ctx_t));
  if (ctx == NULL)
  {
    ESP_LOGE(TAG, "Failed to allocate broadcast work context");
    return;
  }

  // Copy the data
  strncpy(ctx->json_data, json_data, sizeof(ctx->json_data) - 1);
  ctx->json_data[sizeof(ctx->json_data) - 1] = '\0';

  // Queue the work to be executed in HTTP server context
  esp_err_t ret = httpd_queue_work(server, broadcast_work_function, ctx);
  if (ret != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to queue broadcast work: %s", esp_err_to_name(ret));
    free(ctx);
  }
  else
  {
    ESP_LOGD(TAG, "Queued broadcast work for sensor data");
  }
}

// Send initial sensor data to a specific WebSocket client
static void ws_send_initial_data(httpd_req_t *req, int sockfd)
{
  // Get sensor handles
  temp_sensor_handle_t air_sensor = temp_sensor_get_air_sensor();
  temp_sensor_handle_t heater_sensor = temp_sensor_get_heater_sensor();

  // Allocate larger buffer for all historical data (TEMP_BUFFER_SIZE * 2 sensors * ~100 chars per sample)
  size_t buffer_size = (TEMP_BUFFER_SIZE * 2 * 120) + 10; // Extra space for brackets and commas
  char *json_data = malloc(buffer_size);
  if (json_data == NULL)
  {
    ESP_LOGE(TAG, "Failed to allocate memory for initial sensor data");
    return;
  }

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

  esp_err_t ret = httpd_ws_send_frame(req, &ws_resp);
  if (ret != ESP_OK)
  {
    ESP_LOGW(TAG, "Failed to send initial data to WebSocket client fd=%d: %s", sockfd, esp_err_to_name(ret));
  }
  else
  {
    ESP_LOGI(TAG, "Sent initial sensor data to WebSocket client fd=%d (%d samples)", sockfd, count);
  }

  // Free the allocated buffer
  free(json_data);
}

// Send ping frames to all connected WebSocket clients for heartbeat monitoring
static void ws_send_ping_to_clients(void)
{
  if (xSemaphoreTake(ws_clients_mutex, pdMS_TO_TICKS(100)) != pdTRUE)
  {
    ESP_LOGE(TAG, "Failed to take WebSocket clients mutex for ping");
    return;
  }

  httpd_ws_frame_t ping_frame = {
      .final = true,
      .fragmented = false,
      .type = HTTPD_WS_TYPE_PING,
      .payload = NULL,
      .len = 0};

  // Send ping to all clients synchronously since we can't use async outside handler context
  // Note: This may block, but ping frames are small and infrequent
  for (int i = 0; i < ws_client_count; i++)
  {
    int sockfd = ws_client_fds[i];
    // We don't have req context here, so we need to create a dummy req or use a different approach
    // For now, skip ping functionality until we can implement it properly
    ESP_LOGD(TAG, "Ping functionality disabled - requires httpd_req_t context");
  }

  ESP_LOGD(TAG, "Ping skipped for %d clients (no req context)", ws_client_count);
  xSemaphoreGive(ws_clients_mutex);
}

// Broadcast latest sensor readings to all subscribed WebSocket clients
void ws_broadcast_latest_sensor_data(void)
{
  // Check if server is initialized before broadcasting
  if (server == NULL)
  {
    ESP_LOGD(TAG, "Server not initialized yet, skipping broadcast");
    return;
  }

  // Send heartbeat ping before broadcasting data
  ws_send_ping_to_clients();

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
    ESP_LOGI(TAG, "Broadcasting latest sensor data to all clients: %s", json_data);
    ws_broadcast_sensor_data(json_data);
  }
}

// Sensor data handler - WebSocket handler for temperature data with subscription
esp_err_t sensor_data_handler(httpd_req_t *req)
{
  // Get or create session context
  struct ws_session_ctx *sess;
  esp_err_t ret = get_or_create_session_context(req, &sess);
  if (ret != ESP_OK)
  {
    return ret; // ESP_ERR_NO_MEM
  }

  // Handle WebSocket handshake initiation (new connection)
  if (req->method == HTTP_GET)
  {
    ESP_LOGI(TAG, "WebSocket handshake initiated for fd=%d", sess->sockfd);
    // Don't subscribe yet - wait for first frame to confirm handshake completion
    return ESP_OK;
  }

  httpd_ws_frame_t ws_pkt;
  memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));

  // Try to receive a frame
  esp_err_t ws_ret = httpd_ws_recv_frame(req, &ws_pkt, 0);
  if (ws_ret != ESP_OK)
  {
    // Connection likely closed - unsubscribe client
    int sockfd = httpd_req_to_sockfd(req);
    ESP_LOGI(TAG, "WebSocket connection closed for fd=%d", sockfd);
    ws_client_remove(sockfd);

    // Clean up session context
    cleanup_session_context(req, sess);

    return ESP_OK;
  }

  // First successful frame reception confirms handshake is complete
  if (!sess->handshake_complete)
  {
    sess->handshake_complete = true;
    ESP_LOGI(TAG, "WebSocket handshake confirmed for fd=%d, subscribing to sensor updates", sess->sockfd);

    // Subscribe client to sensor data updates now that handshake is complete
    ws_client_add(sess->sockfd);
  }

  ESP_LOGI(TAG, "WS frame type: %d, len: %d, final: %d", ws_pkt.type, ws_pkt.len, ws_pkt.final);

  // Handle frame based on type
  if (ws_pkt.type == HTTPD_WS_TYPE_CLOSE)
  {
    int sockfd = httpd_req_to_sockfd(req);
    ESP_LOGI(TAG, "WebSocket client sent close frame, fd=%d", sockfd);
    ws_client_remove(sockfd);

    // Send close frame back to client
    httpd_ws_frame_t close_frame = {
        .final = true,
        .fragmented = false,
        .type = HTTPD_WS_TYPE_CLOSE,
        .payload = NULL,
        .len = 0};
    httpd_ws_send_frame(req, &close_frame);

    // Clean up session context
    cleanup_session_context(req, sess);

    return ESP_OK;
  }
  else if (ws_pkt.type == HTTPD_WS_TYPE_PONG)
  {
    ESP_LOGD(TAG, "Received pong from client fd=%d", httpd_req_to_sockfd(req));
    // Client is still alive - no action needed
  }
  else if (ws_pkt.type == HTTPD_WS_TYPE_TEXT)
  {
    // Allocate payload buffer for the frame data
    ws_pkt.payload = malloc(ws_pkt.len + 1);
    if (ws_pkt.payload == NULL)
    {
      ESP_LOGE(TAG, "Failed to allocate memory for WS payload");
      return ESP_ERR_NO_MEM;
    }

    // Receive the frame data into our buffer
    ret = httpd_ws_recv_frame(req, &ws_pkt, ws_pkt.len);
    if (ret != ESP_OK)
    {
      ESP_LOGE(TAG, "httpd_ws_recv_frame failed to get data: %s", esp_err_to_name(ret));
      free(ws_pkt.payload);
      return ESP_OK; // Don't close connection
    }

    // Null-terminate the payload for safe string operations
    ((char *)ws_pkt.payload)[ws_pkt.len] = '\0';

    // Process text message
    ESP_LOGI(TAG, "Received text message from client: %s", (char *)ws_pkt.payload);

    // Check if client is requesting initial data
    if (ws_pkt.len == 8 && strcmp((char *)ws_pkt.payload, "get_data") == 0)
    {
      ESP_LOGI(TAG, "Client requested initial data, sending buffered readings");
      ws_send_initial_data(req, sess->sockfd);
    }

    // Free the payload buffer
    free(ws_pkt.payload);
  }
  else if (ws_pkt.type == HTTPD_WS_TYPE_CONTINUE)
  {

    if (sess->accumulating_text)
    {
      // Handle fragmented messages if needed in the future
      // For now, subscription model doesn't expect fragmented messages from clients
      ESP_LOGW(TAG, "Unexpected fragmented message from client");
    }
    else
    {
      if (ws_pkt.payload == NULL)
      {
        ESP_LOGW(TAG, "Found singular CONTINUE frame. Payload=NULL");
      }
      else
      {
        ESP_LOGW(TAG, "Found singular CONTINUE frame. Payload=%s", (char *)ws_pkt.payload);
      }
    }
  }
  // Ignore other frame types

  return ESP_OK;
}
