#ifndef WIFI_H
#define WIFI_H

#include "esp_err.h"
#include "wifi_credentials.h"

// Function declarations
esp_err_t wifi_init(void);
esp_err_t wifi_connect(void);
esp_err_t wifi_wait_for_connection(void);

// Get current IP address as string (returns empty string if not connected)
// Caller must provide buffer of at least 16 characters
esp_err_t wifi_get_ip_address(char *ip_buffer, size_t buffer_size);

#endif // WIFI_H
