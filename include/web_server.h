#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <stdbool.h>
#include <stddef.h>
#include "esp_err.h"
#include "esp_http_server.h"

// Global server handle
extern httpd_handle_t server;

// Function to broadcast latest sensor reading to all WebSocket clients
void ws_broadcast_latest_sensor_data(void);

esp_err_t web_server_init(void);
esp_err_t web_server_start(void);

// Forward declarations for internal use
esp_err_t ws_clients_init(void);
bool custom_uri_match(const char *reference_uri, const char *uri_to_match, size_t match_upto);
esp_err_t version_handler(httpd_req_t *req);
esp_err_t static_file_handler(httpd_req_t *req);
esp_err_t sensor_data_handler(httpd_req_t *req);

#endif // WEB_SERVER_H
