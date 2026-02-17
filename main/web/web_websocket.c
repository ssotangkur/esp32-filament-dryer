#include "web_server.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "circular_buffer.h"
#include "temp.h"
#include "linked_list.h"
#include "wifi.h"
#include "esp_heap_caps.h"
#include <string.h>
#include <stdlib.h>

static const char *TAG = "web_server";

static struct ws_session_ctx *g_first_session = NULL;
static bool g_ws_registered = false;

typedef struct ws_session_ctx ws_session_ctx_t;

#define WS_SEND_TASK_STACK_SIZE 4096
#define WS_SEND_TASK_PRIORITY 3

struct ws_session_ctx
{
  char *buffer;
  size_t len;
  bool accumulating_text;
  int sockfd;
  bool handshake_complete;
  bool running;
  linked_list_t pending_msgs;
  SemaphoreHandle_t send_sem;
  TaskHandle_t send_task_handle;
  ws_session_ctx_t *next;
};

static void ws_send_task(void *arg);

static void air_buffer_callback(const void *data, size_t count, void *user_data)
{
  ws_session_ctx_t *sess = (ws_session_ctx_t *)user_data;
  if (sess == NULL || data == NULL || count == 0)
  {
    return;
  }

  const temp_sample_t *samples = (const temp_sample_t *)data;

  size_t chunk_size = LINKED_LIST_CHUNK_SIZE;
  for (size_t offset = 0; offset < count; offset += chunk_size)
  {
    size_t remaining = count - offset;
    size_t this_chunk = (remaining < chunk_size) ? remaining : chunk_size;

    char *json_buf = heap_caps_malloc(4096, MALLOC_CAP_SPIRAM);
    if (json_buf == NULL)
    {
      ESP_LOGE(TAG, "air_buffer_callback: failed to allocate JSON buffer");
      continue;
    }

    size_t pos = 0;
    pos += snprintf(json_buf + pos, 4096 - pos, "[");

    for (size_t i = 0; i < this_chunk; i++)
    {
      if (i > 0)
      {
        pos += snprintf(json_buf + pos, 4096 - pos, ",");
      }
      pos += snprintf(json_buf + pos, 4096 - pos,
                      "{\"sensor\":\"air\",\"temperature\":%.2f,\"timestamp\":%lu}",
                      samples[offset + i].temperature,
                      (unsigned long)samples[offset + i].timestamp);
    }

    pos += snprintf(json_buf + pos, 4096 - pos, "]");

    if (linked_list_enqueue(&sess->pending_msgs, json_buf, strlen(json_buf) + 1))
    {
      xSemaphoreGive(sess->send_sem);
    }

    heap_caps_free(json_buf);
  }
}

static void heater_buffer_callback(const void *data, size_t count, void *user_data)
{
  ws_session_ctx_t *sess = (ws_session_ctx_t *)user_data;
  if (sess == NULL || data == NULL || count == 0)
  {
    return;
  }

  const temp_sample_t *samples = (const temp_sample_t *)data;

  size_t chunk_size = LINKED_LIST_CHUNK_SIZE;
  for (size_t offset = 0; offset < count; offset += chunk_size)
  {
    size_t remaining = count - offset;
    size_t this_chunk = (remaining < chunk_size) ? remaining : chunk_size;

    char *json_buf = heap_caps_malloc(4096, MALLOC_CAP_SPIRAM);
    if (json_buf == NULL)
    {
      ESP_LOGE(TAG, "heater_buffer_callback: failed to allocate JSON buffer");
      continue;
    }

    size_t pos = 0;
    pos += snprintf(json_buf + pos, 4096 - pos, "[");

    for (size_t i = 0; i < this_chunk; i++)
    {
      if (i > 0)
      {
        pos += snprintf(json_buf + pos, 4096 - pos, ",");
      }
      pos += snprintf(json_buf + pos, 4096 - pos,
                      "{\"sensor\":\"heater\",\"temperature\":%.2f,\"timestamp\":%lu}",
                      samples[offset + i].temperature,
                      (unsigned long)samples[offset + i].timestamp);
    }

    pos += snprintf(json_buf + pos, 4096 - pos, "]");

    if (linked_list_enqueue(&sess->pending_msgs, json_buf, strlen(json_buf) + 1))
    {
      xSemaphoreGive(sess->send_sem);
    }

    heap_caps_free(json_buf);
  }
}

static void ws_send_task(void *arg)
{
  ws_session_ctx_t *sess = (ws_session_ctx_t *)arg;

  ESP_LOGI(TAG, "ws_send_task: started for session %p", (void *)sess);

  while (sess->running)
  {
    if (xSemaphoreTake(sess->send_sem, portMAX_DELAY) != pdTRUE)
    {
      continue;
    }

    if (!sess->running)
    {
      break;
    }

    while (linked_list_count(&sess->pending_msgs) > 0)
    {
      char msg_buf[4096];
      size_t msg_size;

      if (!linked_list_dequeue(&sess->pending_msgs, msg_buf, &msg_size))
      {
        break;
      }

      httpd_ws_frame_t ws_resp = {
          .final = true,
          .fragmented = false,
          .type = HTTPD_WS_TYPE_TEXT,
          .payload = (uint8_t *)msg_buf,
          .len = strlen(msg_buf)};

      httpd_ws_send_frame_async(server, sess->sockfd, &ws_resp);
    }
  }

  ESP_LOGI(TAG, "ws_send_task: exiting for session %p", (void *)sess);
  vTaskDelete(NULL);
}

void ws_broadcast_data(const char *sensor, float temperature)
{
}

static void ws_add_session(ws_session_ctx_t *sess)
{
  ESP_LOGI(TAG, "ws_add_session: adding session %p to list", (void *)sess);
  sess->next = g_first_session;
  g_first_session = sess;
}

static void ws_remove_session(ws_session_ctx_t *sess)
{
  ESP_LOGI(TAG, "ws_remove_session: removing session %p from list", (void *)sess);
  ws_session_ctx_t **ptr = &g_first_session;
  while (*ptr != NULL)
  {
    if (*ptr == sess)
    {
      *ptr = sess->next;
      ESP_LOGI(TAG, "ws_remove_session: session %p removed", (void *)sess);
      return;
    }
    ptr = &(*ptr)->next;
  }
  ESP_LOGW(TAG, "ws_remove_session: session %p not found in list", (void *)sess);
}

static esp_err_t get_or_create_session_context(httpd_req_t *req, ws_session_ctx_t **session_out)
{
  ws_session_ctx_t *sess = (ws_session_ctx_t *)req->sess_ctx;
  if (sess == NULL)
  {
    ESP_LOGI(TAG, "Creating new session context");
    sess = calloc(1, sizeof(ws_session_ctx_t));
    if (sess == NULL)
    {
      ESP_LOGE(TAG, "Failed to allocate session context");
      *session_out = NULL;
      return ESP_ERR_NO_MEM;
    }
    sess->sockfd = -1;
    sess->running = false;

    if (!linked_list_create(&sess->pending_msgs, TEMP_BUFFER_SIZE + 100))
    {
      ESP_LOGE(TAG, "Failed to create pending_msgs linked list");
      free(sess);
      *session_out = NULL;
      return ESP_ERR_NO_MEM;
    }

    sess->send_sem = xSemaphoreCreateBinary();
    if (sess->send_sem == NULL)
    {
      ESP_LOGE(TAG, "Failed to create send semaphore");
      linked_list_destroy(&sess->pending_msgs);
      free(sess);
      *session_out = NULL;
      return ESP_ERR_NO_MEM;
    }

    req->sess_ctx = sess;
    ws_add_session(sess);
    ESP_LOGI(TAG, "Session context created at %p", (void *)sess);
  }

  if (sess->sockfd < 0)
  {
    sess->sockfd = httpd_req_to_sockfd(req);
    ESP_LOGI(TAG, "Assigned sockfd=%d to session %p", sess->sockfd, (void *)sess);
  }

  if (!sess->running && sess->handshake_complete)
  {
    sess->running = true;

    BaseType_t ret = xTaskCreate(ws_send_task, "ws_send", WS_SEND_TASK_STACK_SIZE,
                                  sess, WS_SEND_TASK_PRIORITY, &sess->send_task_handle);
    if (ret != pdPASS)
    {
      ESP_LOGE(TAG, "Failed to create send task");
      sess->running = false;
    }
    else
    {
      circular_buffer_subscribe(temp_sensor_get_air_buffer(), air_buffer_callback, sess);
      circular_buffer_subscribe(temp_sensor_get_heater_buffer(), heater_buffer_callback, sess);
      ESP_LOGI(TAG, "Session %p subscribed to temperature buffers", (void *)sess);
    }
  }

  *session_out = sess;
  return ESP_OK;
}

static void cleanup_session_context(httpd_req_t *req, ws_session_ctx_t *sess)
{
  if (sess == NULL)
  {
    ESP_LOGW(TAG, "cleanup_session_context: sess is NULL");
    return;
  }

  if (sess->sockfd < 0)
  {
    ESP_LOGW(TAG, "cleanup_session_context: sockfd already invalid, possible double-cleanup");
    return;
  }

  int sockfd = sess->sockfd;
  ESP_LOGI(TAG, "cleanup_session_context: removing session from list fd=%d", sockfd);

  if (sess->running)
  {
    sess->running = false;
    xSemaphoreGive(sess->send_sem);

    if (sess->send_task_handle != NULL)
    {
      ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
      sess->send_task_handle = NULL;
    }
  }

  circular_buffer_unsubscribe(temp_sensor_get_air_buffer(), sess);
  circular_buffer_unsubscribe(temp_sensor_get_heater_buffer(), sess);

  linked_list_destroy(&sess->pending_msgs);

  if (sess->send_sem != NULL)
  {
    vSemaphoreDelete(sess->send_sem);
    sess->send_sem = NULL;
  }

  ws_remove_session(sess);
  if (sess->buffer)
  {
    free(sess->buffer);
    sess->buffer = NULL;
  }
  sess->sockfd = -1;
  req->sess_ctx = NULL;
  ESP_LOGI(TAG, "cleanup_session_context: done fd=%d (HTTP server will free memory)", sockfd);
}

esp_err_t ws_clients_init(void)
{
  if (!g_ws_registered)
  {
    ESP_LOGI(TAG, "WebSocket client system initialized (subscribers will be added per-session)");
    g_ws_registered = true;
  }
  return ESP_OK;
}

void ws_broadcast_latest_sensor_data(void)
{
}

static void send_sensor_data_json(httpd_req_t *req, ws_session_ctx_t *sess)
{
  temp_sample_t air_sample;
  temp_sample_t heater_sample;

  if (temp_sensor_get_latest_sample(temp_sensor_get_air_sensor(), &air_sample))
  {
    char json_data[256];
    snprintf(json_data, sizeof(json_data),
             "[{\"sensor\":\"air\",\"temperature\":%.2f,\"timestamp\":%lu}]",
             air_sample.temperature, (unsigned long)air_sample.timestamp);

    httpd_ws_frame_t ws_resp = {
        .final = true,
        .fragmented = false,
        .type = HTTPD_WS_TYPE_TEXT,
        .payload = (uint8_t *)json_data,
        .len = strlen(json_data)};

    httpd_ws_send_frame(req, &ws_resp);
  }

  if (temp_sensor_get_latest_sample(temp_sensor_get_heater_sensor(), &heater_sample))
  {
    char json_data[256];
    snprintf(json_data, sizeof(json_data),
             "[{\"sensor\":\"heater\",\"temperature\":%.2f,\"timestamp\":%lu}]",
             heater_sample.temperature, (unsigned long)heater_sample.timestamp);

    httpd_ws_frame_t ws_resp = {
        .final = true,
        .fragmented = false,
        .type = HTTPD_WS_TYPE_TEXT,
        .payload = (uint8_t *)json_data,
        .len = strlen(json_data)};

    httpd_ws_send_frame(req, &ws_resp);
  }
}

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
    if (ws_pkt.len == 0)
    {
      ESP_LOGD(TAG, "Empty WebSocket text frame received");
      return ESP_OK;
    }

    if (ws_pkt.len > 256)
    {
      ESP_LOGW(TAG, "WebSocket payload too large: %zu", ws_pkt.len);
      cleanup_session_context(req, sess);
      return ESP_ERR_INVALID_SIZE;
    }

    ws_pkt.payload = malloc(ws_pkt.len + 1);
    if (ws_pkt.payload == NULL)
    {
      ESP_LOGE(TAG, "Failed to allocate memory for WS payload");
      cleanup_session_context(req, sess);
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
