#ifndef ESP_LOG_H
#define ESP_LOG_H

// ESP-IDF logging levels
#define ESP_LOG_NONE 0    /*!< No log output */
#define ESP_LOG_ERROR 1   /*!< Critical errors, software module can not recover on its own */
#define ESP_LOG_WARN 2    /*!< Error conditions from which recovery measures have been taken */
#define ESP_LOG_INFO 3    /*!< Information messages which describe normal flow of events */
#define ESP_LOG_DEBUG 4   /*!< Extra information which is not necessary for normal use */
#define ESP_LOG_VERBOSE 5 /*!< Bigger chunks of debugging information */

// ESP-IDF logging macros - these will be no-ops in unit tests
#define ESP_LOGE(tag, format, ...) \
  do                               \
  {                                \
  } while (0)
#define ESP_LOGW(tag, format, ...) \
  do                               \
  {                                \
  } while (0)
#define ESP_LOGI(tag, format, ...) \
  do                               \
  {                                \
  } while (0)
#define ESP_LOGD(tag, format, ...) \
  do                               \
  {                                \
  } while (0)
#define ESP_LOGV(tag, format, ...) \
  do                               \
  {                                \
  } while (0)

// ESP-IDF error to name function
const char *esp_err_to_name(int code);

#endif // ESP_LOG_H
