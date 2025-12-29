#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "driver/adc.h"
#include "sysmon_wrapper.h"

static const char *TAG = "TEMP";

#define TEMP_BUFFER_SIZE 100
#define TEMP_TASK_STACK_SIZE 2048
#define TEMP_TASK_PRIORITY 2
#define TEMP_READ_INTERVAL_MS 1000 // Read temperature every second

// Temperature sample structure
typedef struct
{
  float temperature;
  uint32_t timestamp;
} temp_sample_t;

// Circular buffer in PSRAM
static temp_sample_t *temp_buffer = NULL;
static size_t buffer_head = 0;  // Index for writing new samples
static size_t buffer_count = 0; // Number of valid samples
static SemaphoreHandle_t buffer_mutex = NULL;

// Temperature reading task handle
static TaskHandle_t temp_task_handle = NULL;

/**
 * @brief Temperature reading task
 */
static void temp_task(void *pvParameters)
{
  while (1)
  {
    // Read temperature from ADC
    int adc_reading = adc1_get_raw(ADC1_CHANNEL_0);

    // Convert ADC reading to voltage (approximate: 0-3.3V range)
    float voltage = (adc_reading / 4095.0f) * 3.3f;

    // For demonstration, convert voltage to a simulated temperature reading
    // Assuming 0V = 0°C, 3.3V = 100°C (this would be replaced with actual sensor conversion)
    float temperature = (voltage / 3.3f) * 100.0f;

    // Store sample in circular buffer
    if (xSemaphoreTake(buffer_mutex, portMAX_DELAY) == pdTRUE)
    {
      temp_buffer[buffer_head].temperature = temperature;
      temp_buffer[buffer_head].timestamp = xTaskGetTickCount() * portTICK_PERIOD_MS;

      buffer_head = (buffer_head + 1) % TEMP_BUFFER_SIZE;

      if (buffer_count < TEMP_BUFFER_SIZE)
      {
        buffer_count++;
      }

      ESP_LOGD(TAG, "Stored temperature: %.2f°C (buffer count: %d)", temperature, buffer_count);
      xSemaphoreGive(buffer_mutex);
    }

    // Wait for next reading interval
    vTaskDelay(pdMS_TO_TICKS(TEMP_READ_INTERVAL_MS));
  }
}

/**
 * @brief Initialize ADC and temperature sampling system
 */
void temp_sensor_init(void)
{
  ESP_LOGI(TAG, "Initializing temperature sensor ADC on GPIO1 with circular buffer");

  // Configure ADC1 channel 0 (GPIO1)
  adc1_config_width(ADC_WIDTH_BIT_12);
  adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_DB_11); // 0-3.3V range

  // Allocate circular buffer in PSRAM
  temp_buffer = (temp_sample_t *)heap_caps_malloc(TEMP_BUFFER_SIZE * sizeof(temp_sample_t), MALLOC_CAP_SPIRAM);
  if (temp_buffer == NULL)
  {
    ESP_LOGE(TAG, "Failed to allocate temperature buffer in PSRAM");
    return;
  }

  // Initialize buffer
  memset(temp_buffer, 0, TEMP_BUFFER_SIZE * sizeof(temp_sample_t));
  buffer_head = 0;
  buffer_count = 0;

  // Create mutex for thread-safe access
  buffer_mutex = xSemaphoreCreateMutex();
  if (buffer_mutex == NULL)
  {
    ESP_LOGE(TAG, "Failed to create buffer mutex");
    heap_caps_free(temp_buffer);
    temp_buffer = NULL;
    return;
  }

  // Create temperature reading task (using sysmon wrapper for monitoring)
  BaseType_t result = sysmon_xTaskCreate(
      temp_task,
      "temp_task",
      TEMP_TASK_STACK_SIZE,
      NULL,
      TEMP_TASK_PRIORITY,
      &temp_task_handle);

  if (result != pdPASS)
  {
    ESP_LOGE(TAG, "Failed to create temperature task");
    vSemaphoreDelete(buffer_mutex);
    heap_caps_free(temp_buffer);
    temp_buffer = NULL;
    buffer_mutex = NULL;
    return;
  }

  ESP_LOGI(TAG, "Temperature sensor initialized with %d sample buffer in PSRAM", TEMP_BUFFER_SIZE);
}

/**
 * @brief Get the most recent temperature reading
 * @return Latest temperature in Celsius, or -999.0f if no samples available
 */
float temp_sensor_get_reading(void)
{
  float temperature = -999.0f;

  if (buffer_mutex != NULL && xSemaphoreTake(buffer_mutex, portMAX_DELAY) == pdTRUE)
  {
    if (buffer_count > 0)
    {
      // Get the most recent sample (head - 1, wrapping around)
      size_t latest_index = (buffer_head + TEMP_BUFFER_SIZE - 1) % TEMP_BUFFER_SIZE;
      temperature = temp_buffer[latest_index].temperature;
    }
    xSemaphoreGive(buffer_mutex);
  }

  return temperature;
}

/**
 * @brief Get temperature sample at specific index (0 = oldest, buffer_count-1 = newest)
 * @param index Sample index (0 to buffer_count-1)
 * @return Temperature value, or -999.0f if invalid index
 */
float temp_sensor_get_sample(size_t index)
{
  float temperature = -999.0f;

  if (buffer_mutex != NULL && xSemaphoreTake(buffer_mutex, portMAX_DELAY) == pdTRUE)
  {
    if (index < buffer_count)
    {
      // Calculate buffer index (head - buffer_count + index, wrapping around)
      size_t buffer_index = (buffer_head + TEMP_BUFFER_SIZE - buffer_count + index) % TEMP_BUFFER_SIZE;
      temperature = temp_buffer[buffer_index].temperature;
    }
    xSemaphoreGive(buffer_mutex);
  }

  return temperature;
}

/**
 * @brief Get number of stored temperature samples
 * @return Number of valid samples (0 to TEMP_BUFFER_SIZE)
 */
size_t temp_sensor_get_sample_count(void)
{
  size_t count = 0;

  if (buffer_mutex != NULL && xSemaphoreTake(buffer_mutex, portMAX_DELAY) == pdTRUE)
  {
    count = buffer_count;
    xSemaphoreGive(buffer_mutex);
  }

  return count;
}

/**
 * @brief Deinitialize temperature sensor (cleanup resources)
 */
void temp_sensor_deinit(void)
{
  if (temp_task_handle != NULL)
  {
    vTaskDelete(temp_task_handle);
    temp_task_handle = NULL;
  }

  if (buffer_mutex != NULL)
  {
    vSemaphoreDelete(buffer_mutex);
    buffer_mutex = NULL;
  }

  if (temp_buffer != NULL)
  {
    heap_caps_free(temp_buffer);
    temp_buffer = NULL;
  }

  buffer_head = 0;
  buffer_count = 0;

  ESP_LOGI(TAG, "Temperature sensor deinitialized");
}
