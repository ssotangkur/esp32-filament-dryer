#ifndef UI_STUB_H
#define UI_STUB_H

// Stub definitions for hardware-specific functions used in UI code
// These are used when building for the simulator to avoid dependencies on ESP-IDF

#ifdef BUILD_FOR_SIMULATOR

// Stub for ESP_LOGI macro
#define ESP_LOGI(tag, format, ...) printf("[I][%s] " format "\n", tag, ##__VA_ARGS__)

// Stub for lvgl_port_lock
void lvgl_port_lock(int timeout_ms);

// Stub for lvgl_port_unlock
void lvgl_port_unlock();

#else

// Include real headers when building for ESP32
#include "esp_log.h"
#include "esp_lvgl_port.h"

#endif // BUILD_FOR_SIMULATOR

#endif // UI_STUB_H