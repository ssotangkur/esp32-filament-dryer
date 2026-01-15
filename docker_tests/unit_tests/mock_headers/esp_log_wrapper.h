#ifndef ESP_LOG_WRAPPER_H
#define ESP_LOG_WRAPPER_H

#include <stdarg.h> // Required for va_list

#ifdef __cplusplus
extern "C" {
#endif

// Wrapper for ESP_LOGI
void esp_log_wrapper_i(const char *tag, const char *format, ...);

// Wrapper for ESP_LOGD
void esp_log_wrapper_d(const char *tag, const char *format, ...);

// Wrapper for ESP_LOGW
void esp_log_wrapper_w(const char *tag, const char *format, ...);

// Wrapper for ESP_LOGE
void esp_log_wrapper_e(const char *tag, const char *format, ...);

#ifdef __cplusplus
}
#endif

#endif // ESP_LOG_WRAPPER_H