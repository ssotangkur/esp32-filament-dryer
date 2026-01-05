#include "web_server.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "version.h"
#include <string.h>
#include <stdlib.h>

static const char *TAG = "web_server";

// Custom URI matching function for proper wildcard support
bool custom_uri_match(const char *reference_uri, const char *uri_to_match, size_t match_upto)
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
esp_err_t static_file_handler(httpd_req_t *req)
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
esp_err_t version_handler(httpd_req_t *req)
{
  httpd_resp_set_type(req, "text/plain");
  httpd_resp_sendstr(req, FIRMWARE_VERSION_STRING);
  return ESP_OK;
}
