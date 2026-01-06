#include "web_server.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "circular_buffer.h"
#include "temp.h"
#include <string.h>
#include <stdlib.h>

// Forward declaration for temp_sensor_subscription struct
struct temp_sensor_subscription;

static const char *TAG = "web_server";

// Context structure for sensor callbacks
typedef struct
{
  struct ws_session_ctx *session;
  const char *sensor_type;
} sensor_callback_ctx_t;

// Context structure for queued send work
typedef struct
{
  char json_data[256]; // Copy of the data to send
} send_work_ctx_t;

// WebSocket session context for handling fragmented messages
struct ws_session_ctx
{
  char *buffer;
  size_t len;
  bool accumulating_text;
  int sockfd;                                     // Store the socket fd for reliable access
  bool handshake_complete;                        // Track if WebSocket handshake is fully complete
  temp_sensor_subscription_t air_subscription;    // Air sensor subscription token
  temp_sensor_subscription_t heater_subscription; // Heater sensor subscription token
  sensor_callback_ctx_t *air_context;             // Air sensor callback context (for cleanup)
  sensor_callback_ctx_t *heater_context;          // Heater sensor callback context (for cleanup)
};

/**
 * @brief Generic callback function for sensor updates with per-client context
 * @param sample Pointer to temperature sample (caller owns the memory)
 * @param context Pointer to sensor_callback_ctx_t (callback context)
 */
static void websocket_sensor_callback(temp_sample_t *sample, void *context)
{
  sensor_callback_ctx_t *callback_ctx = (sensor_callback_ctx_t *)context;

  // Validate context and sample
  if (sample == NULL || callback_ctx == NULL || callback_ctx->session == NULL || !callback_ctx->session->handshake_complete)
  {
    return;
  }

  struct ws_session_ctx *session = callback_ctx->session;
  const char *sensor_type = callback_ctx->sensor_type;

  // Format as JSON array with single sample
  char json_data[128];
  snprintf(json_data, sizeof(json_data), "[{\"sensor\":\"%s\",\"temperature\":%.2f,\"timestamp\":%lu}]",
           sensor_type, sample->temperature, (unsigned long)sample->timestamp);

  // Send directly to this specific client
  send_work_ctx_t ctx = {.json_data = {0}};
  strncpy(ctx.json_data, json_data, sizeof(ctx.json_data) - 1);

  // Queue work to send to this specific client
  httpd_ws_frame_t ws_resp = {
      .final = true,
      .fragmented = false,
      .type = HTTPD_WS_TYPE_TEXT,
      .payload = (uint8_t *)ctx.json_data,
      .len = strlen(ctx.json_data)};

  esp_err_t ret = httpd_ws_send_frame_async(server, session->sockfd, &ws_resp);
  if (ret != ESP_OK)
  {
    // Client may have disconnected - log at debug level to avoid spam
    ESP_LOGD(TAG, "Failed to send sensor data to WebSocket client fd=%d: %s (client may have disconnected)", session->sockfd, esp_err_to_name(ret));
    // Don't mark session as disconnected here - let the main handler detect it
  }
  else
  {
    ESP_LOGD(TAG, "Sent sensor data to WebSocket client fd=%d", session->sockfd);
  }
}

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
    // Unsubscribe from sensors before cleanup
    temp_sensor_handle_t air_sensor = temp_sensor_get_air_sensor();
    temp_sensor_handle_t heater_sensor = temp_sensor_get_heater_sensor();

    // Unsubscribe from air sensor and free context
    if (air_sensor != NULL && sess->air_subscription != NULL)
    {
      temp_sensor_unsubscribe(air_sensor, sess->air_subscription);
      sess->air_subscription = NULL;
    }
    if (sess->air_context != NULL)
    {
      free(sess->air_context); // Free the sensor_callback_ctx_t
      sess->air_context = NULL;
    }

    // Unsubscribe from heater sensor and free context
    if (heater_sensor != NULL && sess->heater_subscription != NULL)
    {
      temp_sensor_unsubscribe(heater_sensor, sess->heater_subscription);
      sess->heater_subscription = NULL;
    }
    if (sess->heater_context != NULL)
    {
      free(sess->heater_context); // Free the sensor_callback_ctx_t
      sess->heater_context = NULL;
    }

    // Clean up session context
    if (sess->buffer)
    {
      free(sess->buffer);
    }
    free(sess);
    req->sess_ctx = NULL;
  }
}

// Initialize WebSocket client registry (no-op since we removed the registry)
esp_err_t ws_clients_init(void)
{
  ESP_LOGI(TAG, "WebSocket client registry removed - using per-client callbacks");
  return ESP_OK;
}

// Broadcast latest sensor readings to all subscribed WebSocket clients
void ws_broadcast_latest_sensor_data(void)
{
  // With per-client callbacks, this function is now a no-op
  // Each client receives data through their individual callback subscriptions
  ESP_LOGD(TAG, "ws_broadcast_latest_sensor_data called - data sent via per-client callbacks");
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
    // Connection broken - clean up session context and return error to close connection
    int sockfd = httpd_req_to_sockfd(req);
    ESP_LOGD(TAG, "WebSocket connection broken for fd=%d: %s", sockfd, esp_err_to_name(ws_ret));

    // Clean up session context
    cleanup_session_context(req, sess);

    // Return error to tell HTTP server to close the connection
    return ws_ret;
  }

  // First successful frame reception confirms handshake is complete
  if (!sess->handshake_complete)
  {
    sess->handshake_complete = true;
    ESP_LOGI(TAG, "WebSocket handshake confirmed for fd=%d, subscribing to sensor updates", sess->sockfd);

    // Subscribe to air sensor with session context
    temp_sensor_handle_t air_sensor = temp_sensor_get_air_sensor();
    if (air_sensor != NULL)
    {
      sensor_callback_ctx_t *air_ctx = malloc(sizeof(sensor_callback_ctx_t));
      if (air_ctx != NULL)
      {
        air_ctx->session = sess;
        air_ctx->sensor_type = "air";
        sess->air_context = air_ctx; // Store context for cleanup
        sess->air_subscription = temp_sensor_subscribe(air_sensor, websocket_sensor_callback, air_ctx);
        if (sess->air_subscription == NULL)
        {
          ESP_LOGE(TAG, "Failed to subscribe WebSocket client to air sensor");
          free(air_ctx);
          sess->air_context = NULL;
        }
      }
      else
      {
        ESP_LOGE(TAG, "Failed to allocate air sensor callback context");
      }
    }

    // Subscribe to heater sensor with session context
    temp_sensor_handle_t heater_sensor = temp_sensor_get_heater_sensor();
    if (heater_sensor != NULL)
    {
      sensor_callback_ctx_t *heater_ctx = malloc(sizeof(sensor_callback_ctx_t));
      if (heater_ctx != NULL)
      {
        heater_ctx->session = sess;
        heater_ctx->sensor_type = "heater";
        sess->heater_context = heater_ctx; // Store context for cleanup
        sess->heater_subscription = temp_sensor_subscribe(heater_sensor, websocket_sensor_callback, heater_ctx);
        if (sess->heater_subscription == NULL)
        {
          ESP_LOGE(TAG, "Failed to subscribe WebSocket client to heater sensor");
          free(heater_ctx);
          sess->heater_context = NULL;
        }
      }
      else
      {
        ESP_LOGE(TAG, "Failed to allocate heater sensor callback context");
      }
    }
  }

  ESP_LOGI(TAG, "WS frame type: %d, len: %d, final: %d", ws_pkt.type, ws_pkt.len, ws_pkt.final);

  // Handle frame based on type
  if (ws_pkt.type == HTTPD_WS_TYPE_CLOSE)
  {
    int sockfd = httpd_req_to_sockfd(req);
    ESP_LOGI(TAG, "WebSocket client sent close frame, fd=%d", sockfd);

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
      ESP_LOGD(TAG, "WebSocket frame read failed: %s", esp_err_to_name(ret));
      free(ws_pkt.payload);

      // Clean up session context on socket error
      cleanup_session_context(req, sess);

      // Return error to close connection
      return ret;
    }

    // Null-terminate the payload for safe string operations
    ((char *)ws_pkt.payload)[ws_pkt.len] = '\0';

    // Process text message
    ESP_LOGI(TAG, "Received text message from client: %s", (char *)ws_pkt.payload);

    // Check if client is requesting initial data
    if (ws_pkt.len == 8 && strcmp((char *)ws_pkt.payload, "get_data") == 0)
    {
      ESP_LOGI(TAG, "Got message \"get_data\"");
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
