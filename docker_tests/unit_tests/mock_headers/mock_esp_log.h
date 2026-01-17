#ifndef MOCK_ESP_LOG_H
#define MOCK_ESP_LOG_H

#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

void esp_log_wrapper_i(const char *tag, const char *format, ...);
void esp_log_wrapper_d(const char *tag, const char *format, ...);
void esp_log_wrapper_w(const char *tag, const char *format, ...);
void esp_log_wrapper_e(const char *tag, const char *format, ...);

#ifdef __cplusplus
}
#endif

#endif // MOCK_ESP_LOG_H
