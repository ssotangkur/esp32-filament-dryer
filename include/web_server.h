#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include "esp_err.h"

// Function to broadcast latest sensor reading to all WebSocket clients
void ws_broadcast_latest_sensor_data(void);

esp_err_t web_server_init(void);
esp_err_t web_server_start(void);

#endif // WEB_SERVER_H
