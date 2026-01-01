#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "hal/adc_types.h"
#include "sysmon_wrapper.h"
#include "circular_buffer.h"

static const char *TAG = "TEMP";

#define TEMP_BUFFER_SIZE 100
#define TEMP_TASK_STACK_SIZE 2048
#define TEMP_TASK_PRIORITY 2
#define TEMP_READ_INTERVAL_MS 1000 // Read temperature every second
#define TEMP_AVERAGE_SAMPLES 250   // Number of ADC samples to average for noise reduction

// Thermistor configuration structure
typedef struct
{
  adc_channel_t adc_channel;   // ADC channel for this thermistor
  float nominal_resistance;    // Resistance at 25°C (ohms)
  float nominal_temperature;   // Temperature for nominal resistance (°C)
  float beta_coefficient;      // Beta coefficient (K)
  float series_resistor;       // Series resistor value (ohms)
  float adc_voltage_reference; // ADC reference voltage (V)
  uint16_t averaging_samples;  // Number of ADC samples to average
} thermistor_config_t;

// Default thermistor configuration (can be customized per sensor)
static const thermistor_config_t default_thermistor_config = {
    .adc_channel = ADC_CHANNEL_0,
    .nominal_resistance = 100000.0, // 100kΩ at 25°C
    .nominal_temperature = 25.0,    // 25°C reference temperature
    .beta_coefficient = 3950.0,     // Beta coefficient
    .series_resistor = 100000.0,    // 100kΩ series resistor
    .adc_voltage_reference = 3.3,   // 3.3V ADC reference
    .averaging_samples = TEMP_AVERAGE_SAMPLES};

// Temperature sample structure
typedef struct
{
  float temperature;
  float voltage; // Calibrated and manually adjusted ADC voltage
  uint32_t timestamp;
} temp_sample_t;

// Parameters for temperature reading task
typedef struct
{
  const thermistor_config_t *config; // Thermistor configuration
  circular_buffer_t *buffer;         // Circular buffer for samples
} temp_task_params_t;

// Global temperature buffer for sensor 1 (GPIO1)
static circular_buffer_t temp_buffer_1 = {0};

// ADC oneshot unit handle
static adc_oneshot_unit_handle_t adc1_handle = NULL;

// ADC calibration handle
static adc_cali_handle_t adc_cali_handle = NULL;

/**
 * @brief Calculate temperature from ADC voltage using Steinhart-Hart equation with configuration
 * @param adc_voltage Voltage from ADC (0-3.3V)
 * @param config Pointer to thermistor configuration
 * @return Temperature in Celsius
 */
static float calculate_thermistor_temperature_from_config(float adc_voltage, const thermistor_config_t *config)
{
  // Avoid division by zero
  if (adc_voltage >= config->adc_voltage_reference)
  {
    return -273.15f; // Absolute zero indicates error
  }

  // Calculate thermistor resistance using voltage divider equation
  // R_thermistor = R_series * (V_adc / (V_total - V_adc))
  float thermistor_resistance = config->series_resistor * (adc_voltage / (config->adc_voltage_reference - adc_voltage));

  // Steinhart-Hart equation
  // 1/T = 1/T0 + (1/B) * ln(R/R0)
  // Where:
  //   T = temperature in Kelvin
  //   T0 = nominal temperature in Kelvin (25°C = 298.15K)
  //   B = beta coefficient
  //   R = thermistor resistance
  //   R0 = resistance at T0

  float ln_ratio = log(thermistor_resistance / config->nominal_resistance);
  float reciprocal_temp = (1.0f / (config->nominal_temperature + 273.15f)) +
                          (1.0f / config->beta_coefficient) * ln_ratio;

  // Convert from Kelvin to Celsius
  float temperature_kelvin = 1.0f / reciprocal_temp;
  float temperature_celsius = temperature_kelvin - 273.15f;

  return temperature_celsius;
}

/**
 * @brief Read calibrated voltage from thermistor ADC channel
 * @param config Pointer to thermistor configuration
 * @return Calibrated voltage in volts, or -999.0f on error
 */
static float read_thermistor_voltage(const thermistor_config_t *config)
{
  // Allocate buffer for ADC averaging in PSRAM
  uint16_t *adc_samples = (uint16_t *)heap_caps_malloc(config->averaging_samples * sizeof(uint16_t), MALLOC_CAP_SPIRAM);
  if (adc_samples == NULL)
  {
    ESP_LOGE(TAG, "Failed to allocate ADC averaging buffer in PSRAM");
    return -999.0f;
  }

  // Take multiple ADC samples and collect them for median calculation
  uint16_t valid_samples = 0;

  for (int i = 0; i < config->averaging_samples; i++)
  {
    int adc_reading = 0;
    esp_err_t ret = adc_oneshot_read(adc1_handle, config->adc_channel, &adc_reading);
    if (ret == ESP_OK && adc_reading >= 0 && adc_reading <= 4095)
    {
      adc_samples[valid_samples] = (uint16_t)adc_reading;
      valid_samples++;
    }

    // Small delay between samples to allow ADC stabilization
    vTaskDelay(pdMS_TO_TICKS(1));
  }

  // Calculate median ADC reading (more robust than average for outliers)
  float median_adc_reading = 0.0f;
  if (valid_samples > 0)
  {
    // Sort the samples to find median
    for (int i = 0; i < valid_samples - 1; i++)
    {
      for (int j = 0; j < valid_samples - i - 1; j++)
      {
        if (adc_samples[j] > adc_samples[j + 1])
        {
          // Swap
          uint16_t temp = adc_samples[j];
          adc_samples[j] = adc_samples[j + 1];
          adc_samples[j + 1] = temp;
        }
      }
    }

    // For odd number of samples, take middle value
    // For even number, average the two middle values
    if (valid_samples % 2 == 1)
    {
      median_adc_reading = adc_samples[valid_samples / 2];
    }
    else
    {
      median_adc_reading = (adc_samples[valid_samples / 2 - 1] + adc_samples[valid_samples / 2]) / 2.0f;
    }
  }

  // Convert ADC reading to calibrated voltage
  float voltage = 0.0f;
  if (adc_cali_handle != NULL)
  {
    // Use calibrated conversion if available
    int calibrated_voltage_mv = 0;
    esp_err_t cali_ret = adc_cali_raw_to_voltage(adc_cali_handle, (int)median_adc_reading, &calibrated_voltage_mv);
    if (cali_ret == ESP_OK)
    {
      voltage = calibrated_voltage_mv / 1000.0f; // Convert mV to V
    }
    else
    {
      // Fallback to raw conversion
      voltage = (median_adc_reading / 4095.0f) * config->adc_voltage_reference;
    }
  }
  else
  {
    // Use raw conversion
    voltage = (median_adc_reading / 4095.0f) * config->adc_voltage_reference;
  }

  // Free ADC averaging buffer
  heap_caps_free(adc_samples);

  return voltage;
}

/**
 * @brief Calculate temperature from calibrated voltage using Steinhart-Hart equation
 * @param voltage Calibrated voltage from thermistor (0-3.3V)
 * @param config Pointer to thermistor configuration
 * @return Temperature in Celsius
 */
static float calculate_temperature_from_voltage(float voltage, const thermistor_config_t *config)
{
  // Calculate temperature using Steinhart-Hart equation for thermistor
  float temperature = calculate_thermistor_temperature_from_config(voltage, config);

  // Check for invalid readings
  if (temperature < -50.0f || temperature > 150.0f)
  {
    ESP_LOGW(TAG, "Invalid temperature reading: %.2f°C (Voltage: %.3fV)", temperature, voltage);
    temperature = -999.0f; // Mark as invalid
  }
  else
  {
    ESP_LOGD(TAG, "Calculated temperature: %.2f°C (Voltage: %.3fV)", temperature, voltage);
  }

  return temperature;
}

// Temperature reading task handle
static TaskHandle_t temp_task_handle = NULL;

/**
 * @brief Generic temperature reading task
 * @param pvParameters Pointer to temp_task_params_t structure
 */
static void temp_task(void *pvParameters)
{
  // Parameters contain thermistor config and buffer pointer
  temp_task_params_t *params = (temp_task_params_t *)pvParameters;
  const thermistor_config_t *config = params->config;
  circular_buffer_t *buffer = params->buffer;

  while (1)
  {
    // Read calibrated voltage from thermistor ADC
    float voltage = read_thermistor_voltage(config);

    // Calculate temperature from the calibrated voltage
    float temperature = calculate_temperature_from_voltage(voltage, config);

    // Create temperature sample with temperature, voltage, and timestamp
    temp_sample_t sample = {
        .temperature = temperature,
        .voltage = voltage,
        .timestamp = xTaskGetTickCount() * portTICK_PERIOD_MS};

    // Record the temperature sample in the buffer
    circular_buffer_push(buffer, &sample);

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
           default_thermistor_config.nominal_resistance, default_thermistor_config.nominal_temperature,
           default_thermistor_config.beta_coefficient, default_thermistor_config.series_resistor);
  ESP_LOGI(TAG, "ADC averaging: %d samples for noise reduction", default_thermistor_config.averaging_samples);

  // Initialize ADC1 oneshot unit
  adc_oneshot_unit_init_cfg_t init_config = {
      .unit_id = ADC_UNIT_1,
      .ulp_mode = ADC_ULP_MODE_DISABLE,
  };
  esp_err_t ret = adc_oneshot_new_unit(&init_config, &adc1_handle);
  if (ret != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to initialize ADC oneshot unit: %s", esp_err_to_name(ret));
    return;
  }

  // Initialize ADC calibration
  adc_cali_curve_fitting_config_t cali_config = {
      .unit_id = ADC_UNIT_1,
      .atten = ADC_ATTEN_DB_12,
      .bitwidth = ADC_BITWIDTH_12,
  };
  ret = adc_cali_create_scheme_curve_fitting(&cali_config, &adc_cali_handle);
  if (ret == ESP_OK)
  {
    ESP_LOGI(TAG, "ADC calibration enabled using curve fitting scheme");
  }
  else
  {
    ESP_LOGW(TAG, "ADC calibration failed (%s) - using raw ADC values", esp_err_to_name(ret));
    ESP_LOGW(TAG, "Consider manual calibration for better accuracy");
  }

  // Log calibration voltage range for debugging
  if (adc_cali_handle != NULL)
  {
    int min_voltage = 0, max_voltage = 0;
    adc_cali_raw_to_voltage(adc_cali_handle, 0, &min_voltage);
    adc_cali_raw_to_voltage(adc_cali_handle, 4095, &max_voltage);
    ESP_LOGI(TAG, "ADC calibration range: %d mV to %d mV", min_voltage, max_voltage);
  }
  else
  {
    ESP_LOGI(TAG, "ADC calibration not available - using raw values");
  }

  // Configure ADC1 channel 0 (GPIO1)
  adc_oneshot_chan_cfg_t config = {
      .atten = ADC_ATTEN_DB_12, // 0-3.3V range
      .bitwidth = ADC_BITWIDTH_12,
  };
  ret = adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_0, &config);
  if (ret != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to configure ADC channel: %s", esp_err_to_name(ret));
    adc_oneshot_del_unit(adc1_handle);
    adc1_handle = NULL;
    return;
  }

  // Initialize temperature buffer
  if (!circular_buffer_init(&temp_buffer_1, sizeof(temp_sample_t), TEMP_BUFFER_SIZE))
  {
    ESP_LOGE(TAG, "Failed to initialize temperature buffer");
    return;
  }

  // Create task parameters
  static temp_task_params_t task_params = {
      .config = &default_thermistor_config,
      .buffer = &temp_buffer_1};

  // Create temperature reading task (using sysmon wrapper for monitoring)
  BaseType_t result = sysmon_xTaskCreate(
      temp_task,
      "temp_task",
      TEMP_TASK_STACK_SIZE,
      &task_params,
      TEMP_TASK_PRIORITY,
      &temp_task_handle);

  if (result != pdPASS)
  {
    ESP_LOGE(TAG, "Failed to create temperature task");
    circular_buffer_free(&temp_buffer_1);
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
  temp_sample_t sample;
  if (circular_buffer_get_latest(&temp_buffer_1, &sample))
  {
    return sample.temperature;
  }
  return -999.0f;
}

/**
 * @brief Get temperature sample at specific index (0 = oldest, buffer_count-1 = newest)
 * @param index Sample index (0 to buffer_count-1)
 * @return Temperature value, or -999.0f if invalid index
 */
float temp_sensor_get_sample(size_t index)
{
  temp_sample_t sample;
  if (circular_buffer_get_at_index(&temp_buffer_1, index, &sample))
  {
    return sample.temperature;
  }
  return -999.0f;
}

/**
 * @brief Get number of stored temperature samples
 * @return Number of valid samples (0 to TEMP_BUFFER_SIZE)
 */
size_t temp_sensor_get_sample_count(void)
{
  return circular_buffer_count(&temp_buffer_1);
}

/**
 * @brief Get the most recent ADC voltage reading (calibrated and manually adjusted)
 * @return Latest voltage in volts, or -999.0f if no samples available
 */
float temp_sensor_get_voltage(void)
{
  temp_sample_t sample;
  if (circular_buffer_get_latest(&temp_buffer_1, &sample))
  {
    return sample.voltage;
  }
  return -999.0f;
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

  circular_buffer_free(&temp_buffer_1);

  if (adc_cali_handle != NULL)
  {
    adc_cali_delete_scheme_curve_fitting(adc_cali_handle);
    adc_cali_handle = NULL;
  }

  if (adc1_handle != NULL)
  {
    adc_oneshot_del_unit(adc1_handle);
    adc1_handle = NULL;
  }

  ESP_LOGI(TAG, "Temperature sensor deinitialized");
}

// Note: ADC averaging buffer is freed by the task itself when it exits
