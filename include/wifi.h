#pragma once

#include "esp_err.h"
#include "wifi_credentials.h"
#include <stdint.h>

// Function declarations
esp_err_t wifi_init(void);
esp_err_t wifi_connect(void);
esp_err_t wifi_wait_for_connection(void);
void wifi_sync_time(void);

// Get current time as milliseconds since epoch (0 if NTP not synced)
uint64_t wifi_get_epoch_ms(void);

// Get current IP address as string (returns empty string if not connected)
// Caller must provide buffer of at least 16 characters
esp_err_t wifi_get_ip_address(char *ip_buffer, size_t buffer_size);
