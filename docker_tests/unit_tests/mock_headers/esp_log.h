#pragma once

#include <stdio.h>
#include <stdarg.h>

// ESP-IDF logging levels
#define ESP_LOG_NONE 0    /*!< No log output */
#define ESP_LOG_ERROR 1   /*!< Critical errors, software module can not recover on its own */
#define ESP_LOG_WARN 2    /*!< Error conditions from which recovery measures have been taken */
#define ESP_LOG_INFO 3    /*!< Information messages which describe normal flow of events */
#define ESP_LOG_DEBUG 4   /*!< Extra information which is not necessary for normal use */
#define ESP_LOG_VERBOSE 5 /*!< Bigger chunks of debugging information */

// Helper function to avoid format warnings in unit tests
static inline void esp_log_printf(const char *level, const char *tag, const char *format, ...)
{
  va_list args;
  printf("[%s] %s: ", level, tag);
  va_start(args, format);
  vprintf(format, args);
  va_end(args);
  printf("\n");
}

// ESP-IDF logging macros - these will print visible output in unit tests
#define ESP_LOGE(tag, format, ...) esp_log_printf("ERROR", tag, format, ##__VA_ARGS__)
#define ESP_LOGW(tag, format, ...) esp_log_printf("WARN", tag, format, ##__VA_ARGS__)
#define ESP_LOGI(tag, format, ...) esp_log_printf("INFO", tag, format, ##__VA_ARGS__)
#define ESP_LOGD(tag, format, ...) esp_log_printf("DEBUG", tag, format, ##__VA_ARGS__)
#define ESP_LOGV(tag, format, ...) esp_log_printf("VERBOSE", tag, format, ##__VA_ARGS__)

// ESP-IDF error to name function
const char *esp_err_to_name(int code);
