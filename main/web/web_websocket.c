#include "web_server.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "circular_buffer.h"
#include "temp.h"
#include "ui/subjects.h"
#include <string.h>
#include <stdlib.h>

static const char *TAG = "web_server";

static struct ws_session_ctx *g_first_session = NULL;
static bool g_ws_registered = false;

typedef struct ws_session_ctx ws_session_ctx_t;

struct ws_session_ctx
{
  char *buffer;
  size_t len;
  bool accumulating_text;
  int sockfd;
  bool handshake_complete;
  ws_session_ctx_t *next;
};

/**
 * @brief Broadcast temperature data to all connected WebSocket clients
 * @param air_temp Current air temperature reading
 * @param heater_temp Current heater temperature reading
 * @note Formats JSON payload and sends to all sessions with completed handshakes
 */
static void ws_broadcast_temperature(float air_temp, float heater_temp)
{
  char json_data[256];
  snprintf(json_data, sizeof(json_data), "[{\"sensor\":\"air\",\"temperature\":%.2f},{\"sensor\":\"heater\",\"temperature\":%.2f}]",
           air_temp, heater_temp);

  httpd_ws_frame_t ws_resp = {
      .final = true,
      .fragmented = false,
      .type = HTTPD_WS_TYPE_TEXT,
      .payload = (uint8_t *)json_data,
      .len = strlen(json_data)};

  ws_session_ctx_t *sess = g_first_session;
  while (sess != NULL)
  {
    if (sess->handshake_complete)
    {
      httpd_ws_send_frame_async(server, sess->sockfd, &ws_resp);
    }
    sess = sess->next;
  }
}

/**
 * @brief LVGL observer callback for air temperature subject changes
 * @param observer Pointer to the observer (unused)
 * @param subject Pointer to the air temperature subject
 * @note Retrieves current heater temp and broadcasts both temperatures
 */
static void air_temp_subject_callback(lv_observer_t *observer, lv_subject_t *subject)
{
  float air_temp = lv_subject_get_float(subject);
  temp_sensor_handle_t heater_sensor = temp_sensor_get_heater_sensor();
  float heater_temp = temp_sensor_get_reading(heater_sensor);
  ws_broadcast_temperature(air_temp, heater_temp);
}

/**
 * @brief LVGL observer callback for heater temperature subject changes
 * @param observer Pointer to the observer (unused)
 * @param subject Pointer to the heater temperature subject
 * @note Retrieves current air temp and broadcasts both temperatures
 */
static void heater_temp_subject_callback(lv_observer_t *observer, lv_subject_t *subject)
{
  float heater_temp = lv_subject_get_float(subject);
  temp_sensor_handle_t air_sensor = temp_sensor_get_air_sensor();
  float air_temp = temp_sensor_get_reading(air_sensor);
  ws_broadcast_temperature(air_temp, heater_temp);
}

/**
 * @brief Add a WebSocket session to the global linked list
 * @param sess Session to add
 * @note Inserts at head of list for O(1) insertion
 */
static void ws_add_session(ws_session_ctx_t *sess)
{
  sess->next = g_first_session;
  g_first_session = sess;
}

/**
 * @brief Remove a WebSocket session from the global linked list
 * @param sess Session to remove
 * @note Handles head, middle, and end of list removal
 */
static void ws_remove_session(ws_session_ctx_t *sess)
{
  ws_session_ctx_t **ptr = &g_first_session;
  while (*ptr != NULL)
  {
    if (*ptr == sess)
    {
      *ptr = sess->next;
      return;
    }
    ptr = &(*ptr)->next;
  }
}

/**
 * @brief Get or create WebSocket session context for a request
 * @param req HTTP request
 * @param[out] session_out Pointer to store session context
 * @return ESP_OK on success, ESP_ERR_NO_MEM on allocation failure
 * @note Stores session in req->sess_ctx for persistence across requests
 */
static esp_err_t get_or_create_session_context(httpd_req_t *req, ws_session_ctx_t **session_out)
{
  ws_session_ctx_t *sess = (ws_session_ctx_t *)req->sess_ctx;
  if (sess == NULL)
  {
    sess = calloc(1, sizeof(ws_session_ctx_t));
    if (sess == NULL)
    {
      ESP_LOGE(TAG, "Failed to allocate session context");
      *session_out = NULL;
      return ESP_ERR_NO_MEM;
    }
    req->sess_ctx = sess;
    sess->sockfd = httpd_req_to_sockfd(req);
    ws_add_session(sess);
  }

  *session_out = sess;
  return ESP_OK;
}

/**
 * @brief Clean up and free a WebSocket session context
 * @param req HTTP request
 * @param sess Session to clean up
 * @note Removes session from linked list and frees all allocated memory
 */
static void cleanup_session_context(httpd_req_t *req, ws_session_ctx_t *sess)
{
  if (sess != NULL)
  {
    ws_remove_session(sess);
    if (sess->buffer)
    {
      free(sess->buffer);
    }
    free(sess);
    req->sess_ctx = NULL;
  }
}

/**
 * @brief Initialize WebSocket client registry and subscribe to temperature subjects
 * @return ESP_OK on success
 * @note Registers callbacks with LVGL subjects; only runs once (g_ws_registered guard)
 */
esp_err_t ws_clients_init(void)
{
  if (!g_ws_registered)
  {
    lv_subject_add_observer(&g_subject_air_temp, air_temp_subject_callback, NULL);
    lv_subject_add_observer(&g_subject_heater_temp, heater_temp_subject_callback, NULL);
    g_ws_registered = true;
    ESP_LOGI(TAG, "WebSocket subscribed to temperature subjects");
  }
  return ESP_OK;
}

/**
 * @brief Broadcast current sensor readings to all WebSocket clients
 * @note Used for explicit data requests ("get_data" message)
 */
void ws_broadcast_latest_sensor_data(void)
{
  float air_temp = lv_subject_get_float(&g_subject_air_temp);
  float heater_temp = lv_subject_get_float(&g_subject_heater_temp);
  ws_broadcast_temperature(air_temp, heater_temp);
}

/**
 * @brief Send temperature data as JSON to a specific WebSocket client
 * @param req HTTP request
 * @param sess Session to send to
 * @note Used for responding to "get_data" requests from clients
 */
static void send_sensor_data_json(httpd_req_t *req, ws_session_ctx_t *sess)
{
  float air_temp = lv_subject_get_float(&g_subject_air_temp);
  float heater_temp = lv_subject_get_float(&g_subject_heater_temp);

  char json_data[256];
  snprintf(json_data, sizeof(json_data), "[{\"sensor\":\"air\",\"temperature\":%.2f},{\"sensor\":\"heater\",\"temperature\":%.2f}]",
           air_temp, heater_temp);

  httpd_ws_frame_t ws_resp = {
      .final = true,
      .fragmented = false,
      .type = HTTPD_WS_TYPE_TEXT,
      .payload = (uint8_t *)json_data,
      .len = strlen(json_data)};

  esp_err_t ret = httpd_ws_send_frame(req, &ws_resp);
  if (ret != ESP_OK)
  {
    ESP_LOGD(TAG, "Failed to send sensor data to WebSocket client fd=%d", sess->sockfd);
  }
}

/**
 * @brief Main WebSocket handler for sensor data endpoint
 * @param req HTTP request
 * @return ESP_OK on success
 * @note Handles WebSocket handshake, frames, and session lifecycle
 */
esp_err_t sensor_data_handler(httpd_req_t *req)
{
  ws_session_ctx_t *sess;
  esp_err_t ret = get_or_create_session_context(req, &sess);
  if (ret != ESP_OK)
  {
    return ret;
  }

  if (req->method == HTTP_GET)
  {
    ESP_LOGI(TAG, "WebSocket handshake initiated for fd=%d", sess->sockfd);
    return ESP_OK;
  }

  httpd_ws_frame_t ws_pkt;
  memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));

  esp_err_t ws_ret = httpd_ws_recv_frame(req, &ws_pkt, 0);
  if (ws_ret != ESP_OK)
  {
    ESP_LOGD(TAG, "WebSocket connection broken for fd=%d: %s", sess->sockfd, esp_err_to_name(ws_ret));
    cleanup_session_context(req, sess);
    return ws_ret;
  }

  if (!sess->handshake_complete)
  {
    sess->handshake_complete = true;
    ESP_LOGI(TAG, "WebSocket handshake confirmed for fd=%d", sess->sockfd);
  }

  if (ws_pkt.type == HTTPD_WS_TYPE_CLOSE)
  {
    int sockfd = httpd_req_to_sockfd(req);
    ESP_LOGI(TAG, "WebSocket client sent close frame, fd=%d", sockfd);

    httpd_ws_frame_t close_frame = {
        .final = true,
        .fragmented = false,
        .type = HTTPD_WS_TYPE_CLOSE,
        .payload = NULL,
        .len = 0};
    httpd_ws_send_frame(req, &close_frame);

    cleanup_session_context(req, sess);
    return ESP_OK;
  }
  else if (ws_pkt.type == HTTPD_WS_TYPE_TEXT)
  {
    ws_pkt.payload = malloc(ws_pkt.len + 1);
    if (ws_pkt.payload == NULL)
    {
      ESP_LOGE(TAG, "Failed to allocate memory for WS payload");
      return ESP_ERR_NO_MEM;
    }

    ret = httpd_ws_recv_frame(req, &ws_pkt, ws_pkt.len);
    if (ret != ESP_OK)
    {
      ESP_LOGD(TAG, "WebSocket frame read failed: %s", esp_err_to_name(ret));
      free(ws_pkt.payload);
      cleanup_session_context(req, sess);
      return ret;
    }

    ((char *)ws_pkt.payload)[ws_pkt.len] = '\0';

    if (ws_pkt.len == 8 && strcmp((char *)ws_pkt.payload, "get_data") == 0)
    {
      send_sensor_data_json(req, sess);
    }

    free(ws_pkt.payload);
  }

  return ESP_OK;
}
