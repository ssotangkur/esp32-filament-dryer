#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
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
#define TEMP_AVERAGE_SAMPLES 100   // Number of ADC samples to average for noise reduction

// Thermistor constants (adjust these for your specific thermistor)
#define THERMISTOR_NOMINAL_RESISTANCE 100000.0 // Resistance at 25°C (10kΩ typical)
#define THERMISTOR_NOMINAL_TEMPERATURE 25.0    // Temperature for nominal resistance
#define THERMISTOR_BETA_COEFFICIENT 3950.0     // Beta coefficient (3950K typical)
#define SERIES_RESISTOR 100000.0               // Series resistor value (10kΩ typical)
#define ADC_VOLTAGE_REFERENCE 3.3              // ADC reference voltage

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

/**
 * @brief Calculate temperature from ADC voltage using Steinhart-Hart equation
 * @param adc_voltage Voltage from ADC (0-3.3V)
 * @return Temperature in Celsius
 */
static float calculate_thermistor_temperature(float adc_voltage)
{
  // Avoid division by zero
  if (adc_voltage >= ADC_VOLTAGE_REFERENCE)
  {
    return -273.15f; // Absolute zero indicates error
  }

  // Calculate thermistor resistance using voltage divider equation
  // R_thermistor = R_series * (V_adc / (V_total - V_adc))
  float thermistor_resistance = SERIES_RESISTOR * (adc_voltage / (ADC_VOLTAGE_REFERENCE - adc_voltage));

  // Steinhart-Hart equation
  // 1/T = 1/T0 + (1/B) * ln(R/R0)
  // Where:
  //   T = temperature in Kelvin
  //   T0 = nominal temperature in Kelvin (25°C = 298.15K)
  //   B = beta coefficient
  //   R = thermistor resistance
  //   R0 = resistance at T0

  float ln_ratio = log(thermistor_resistance / THERMISTOR_NOMINAL_RESISTANCE);
  float reciprocal_temp = (1.0f / (THERMISTOR_NOMINAL_TEMPERATURE + 273.15f)) +
                          (1.0f / THERMISTOR_BETA_COEFFICIENT) * ln_ratio;

  // Convert from Kelvin to Celsius
  float temperature_kelvin = 1.0f / reciprocal_temp;
  float temperature_celsius = temperature_kelvin - 273.15f;

  return temperature_celsius;
}

// Temperature reading task handle
static TaskHandle_t temp_task_handle = NULL;

/**
 * @brief Temperature reading task
 */
static void temp_task(void *pvParameters)
{
  // Allocate buffer for ADC averaging in PSRAM
  uint16_t *adc_samples = (uint16_t *)heap_caps_malloc(TEMP_AVERAGE_SAMPLES * sizeof(uint16_t), MALLOC_CAP_SPIRAM);
  if (adc_samples == NULL)
  {
    ESP_LOGE(TAG, "Failed to allocate ADC averaging buffer in PSRAM");
    vTaskDelete(NULL);
    return;
  }

  while (1)
  {
    // Take multiple ADC samples and average them to reduce noise
    uint32_t adc_sum = 0;
    uint16_t valid_samples = 0;

    for (int i = 0; i < TEMP_AVERAGE_SAMPLES; i++)
    {
      int adc_reading = adc1_get_raw(ADC1_CHANNEL_0);
      if (adc_reading >= 0 && adc_reading <= 4095)
      {
        adc_samples[valid_samples] = (uint16_t)adc_reading;
        adc_sum += adc_reading;
        valid_samples++;
      }

      // Small delay between samples to allow ADC stabilization
      vTaskDelay(pdMS_TO_TICKS(1));
    }

    // Calculate average ADC reading
    float avg_adc_reading = (valid_samples > 0) ? (float)adc_sum / valid_samples : 0.0f;

    // Convert ADC reading to voltage
    float voltage = (avg_adc_reading / 4095.0f) * ADC_VOLTAGE_REFERENCE;

    // Calculate temperature using Steinhart-Hart equation for thermistor
    float temperature = calculate_thermistor_temperature(voltage);

    // Check for invalid readings
    if (temperature < -50.0f || temperature > 150.0f)
    {
      ESP_LOGW(TAG, "Invalid temperature reading: %.2f°C (Avg ADC: %.1f, Voltage: %.3fV, Samples: %d)",
               temperature, avg_adc_reading, voltage, valid_samples);
      temperature = -999.0f; // Mark as invalid
    }
    else
    {
      ESP_LOGD(TAG, "Temperature: %.2f°C (Avg ADC: %.1f, Voltage: %.3fV, Samples: %d)",
               temperature, avg_adc_reading, voltage, valid_samples);
    }

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
  ESP_LOGI(TAG, "Initializing thermistor temperature sensor (Steinhart-Hart) ADC on GPIO1 with circular buffer");
  ESP_LOGI(TAG, "Thermistor: %.0fΩ @ %.0f°C, Beta=%.0fK, Series R=%.0fΩ",
           THERMISTOR_NOMINAL_RESISTANCE, THERMISTOR_NOMINAL_TEMPERATURE,
           THERMISTOR_BETA_COEFFICIENT, SERIES_RESISTOR);
  ESP_LOGI(TAG, "ADC averaging: %d samples for noise reduction", TEMP_AVERAGE_SAMPLES);

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

// Note: ADC averaging buffer is freed by the task itself when it exits
