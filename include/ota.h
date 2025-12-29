#ifndef OTA_H
#define OTA_H

#include "esp_err.h"
#include <stdbool.h>

/**
 * @brief Initialize OTA update functionality
 *
 * This function sets up the OTA update mechanism including mutexes
 * and rollback protection. Call this once during system initialization.
 *
 * @return ESP_OK on success, or an error code on failure
 */
esp_err_t ota_init(void);

/**
 * @brief Start automatic periodic OTA checking
 *
 * This function starts a background task that checks for firmware updates
 * every 5 seconds and automatically installs them when available.
 * Should be called after WiFi connection is established.
 *
 * @return ESP_OK on success, or an error code on failure
 */
esp_err_t ota_start_auto_check(void);

/**
 * @brief Start an OTA update from a given HTTPS URL
 *
 * This function downloads and installs a firmware update from the specified HTTPS URL.
 * Use this for internet-based updates with security.
 *
 * @param url The HTTPS URL of the firmware binary to download
 * @return ESP_OK on success, or an error code on failure
 */
esp_err_t ota_update_from_https_url(const char *url);

/**
 * @brief Start an OTA update from a given HTTP URL (local network)
 *
 * This function downloads and installs a firmware update from the specified HTTP URL.
 * Use this for local network updates where security is less critical.
 *
 * @param url The HTTP URL of the firmware binary to download
 * @return ESP_OK on success, or an error code on failure
 */
esp_err_t ota_update_from_http_url(const char *url);

/**
 * @brief Start an OTA update from a given URL (auto-detect HTTP/HTTPS)
 *
 * This function automatically detects whether to use HTTP or HTTPS based on the URL.
 *
 * @param url The URL of the firmware binary to download
 * @return ESP_OK on success, or an error code on failure
 */
esp_err_t ota_update_from_url(const char *url);

/**
 * @brief Check if an OTA update is currently in progress
 *
 * @return true if OTA update is in progress, false otherwise
 */
bool ota_is_updating(void);

/**
 * @brief Check if a firmware update is available
 *
 * This function compares the current firmware version with the version
 * available on the server. Returns true if an update is available.
 *
 * @param ota_url The base OTA URL (without /firmware.bin)
 * @return true if update is available, false otherwise
 */
bool ota_check_for_update(const char *ota_url);

/**
 * @brief Get the current OTA update progress (0-100)
 *
 * @return Progress percentage, or -1 if no update in progress
 */
int ota_get_progress(void);

#endif // OTA_H
