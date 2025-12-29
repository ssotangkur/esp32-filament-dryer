#ifndef WIFI_H
#define WIFI_H

#include "esp_err.h"
#include "wifi_credentials.h"

// Function declarations
esp_err_t wifi_init(void);
esp_err_t wifi_connect(void);
esp_err_t wifi_wait_for_connection(void);

#endif // WIFI_H
