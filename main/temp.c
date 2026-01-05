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
#include "temp.h"
#include "web_server.h"

static const char *TAG = "TEMP";

// Thermistor configuration structure
typedef struct
{
  adc_channel_t adc_channel;      // ADC channel for this thermistor
  steinhart_hart_coeffs_t coeffs; // Steinhart-Hart coefficients
  float series_resistor;          // Series resistor value (ohms)
  float adc_voltage_reference;    // ADC reference voltage (V)
  uint16_t averaging_samples;     // Number of ADC samples to average
} thermistor_config_t;

// Sensor information for the task
typedef struct
{
  const thermistor_config_t *config; // Thermistor configuration
  circular_buffer_t *buffer;         // Circular buffer for samples
} sensor_info_t;

// Parameters for temperature reading task
typedef struct
{
  sensor_info_t *sensors; // Array of sensor configurations
  size_t sensor_count;    // Number of sensors
} temp_task_params_t;

// Temperature sensor handle structure (opaque type implementation)
struct temp_sensor_handle
{
  circular_buffer_t *buffer;         // Pointer to the sensor's buffer
  const thermistor_config_t *config; // Pointer to the sensor's configuration (optional, for future use)
};

// Global temperature buffers
static circular_buffer_t temp_buffer_1 = {0}; // Air temperature (ADC_CHANNEL_0)
static circular_buffer_t temp_buffer_2 = {0}; // Heater temperature (ADC_CHANNEL_1)

// Global sensor configurations (set during init)
static thermistor_config_t *air_config_ptr = NULL;
static thermistor_config_t *heater_config_ptr = NULL;

// Global sensor handles (initialized at runtime)
static struct temp_sensor_handle air_sensor_handle;
static struct temp_sensor_handle heater_sensor_handle;

// ADC oneshot unit handle
static adc_oneshot_unit_handle_t adc1_handle = NULL;

// ADC calibration handle
static adc_cali_handle_t adc_cali_handle = NULL;

/**
 * @brief Calculate thermistor resistance from ADC voltage using voltage divider equation
 * @param adc_voltage Voltage from ADC (0-3.3V)
 * @param config Pointer to thermistor configuration
 * @return Thermistor resistance in ohms, or -999.0f on error
 */
static float calculate_thermistor_resistance(float adc_voltage, const thermistor_config_t *config)
{
  // Avoid division by zero
  if (adc_voltage >= config->adc_voltage_reference || adc_voltage < 0.0f)
  {
    return -999.0f; // Invalid voltage
  }

  // Calculate thermistor resistance using voltage divider equation
  // R_thermistor = R_series * (V_adc / (V_total - V_adc))
  float thermistor_resistance = config->series_resistor * (adc_voltage / (config->adc_voltage_reference - adc_voltage));

  return thermistor_resistance;
}

/**
 * @brief Calculate temperature from thermistor resistance using Steinhart-Hart equation
 * @param thermistor_resistance Thermistor resistance in ohms
 * @param config Pointer to thermistor configuration
 * @return Temperature in Celsius
 */
static float calculate_temperature_from_resistance(float thermistor_resistance, const thermistor_config_t *config)
{
  if (thermistor_resistance <= 0.0f)
  {
    return -273.15f; // Absolute zero indicates error
  }

  // Full Steinhart-Hart equation
  // 1/T = A + B * ln(R) + C * (ln(R))^3
  // Where:
  //   T = temperature in Kelvin
  //   R = thermistor resistance
  //   A, B, C = Steinhart-Hart coefficients

  float ln_r = logf(thermistor_resistance);
  float reciprocal_temp = config->coeffs.A + config->coeffs.B * ln_r + config->coeffs.C * ln_r * ln_r * ln_r;

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
  // Allocate buffer for calculating ADC median in PSRAM
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

  // Free ADC median buffer
  heap_caps_free(adc_samples);

  return voltage;
}

// Temperature reading task handle
static TaskHandle_t temp_task_handle = NULL;

/**
 * @brief Multi-sensor temperature reading task
 * Reads all sensors passed via parameters in a loop
 * @param pvParameters Pointer to temp_task_params_t with sensor array
 */
static void temp_task(void *pvParameters)
{
  temp_task_params_t *params = (temp_task_params_t *)pvParameters;
  if (params == NULL || params->sensors == NULL || params->sensor_count == 0)
  {
    ESP_LOGE(TAG, "Invalid parameters for temp_task");
    return;
  }

  while (1)
  {
    // Process each sensor in the array
    for (size_t i = 0; i < params->sensor_count; i++)
    {
      sensor_info_t *sensor = &params->sensors[i];

      if (sensor->config != NULL && sensor->buffer != NULL)
      {
        // Read voltage from this sensor
        float voltage = read_thermistor_voltage(sensor->config);

        // Calculate thermistor resistance from voltage
        float resistance = calculate_thermistor_resistance(voltage, sensor->config);

        // Calculate temperature from resistance using Steinhart-Hart equation
        float temperature = calculate_temperature_from_resistance(resistance, sensor->config);

        // Check for invalid readings
        if (temperature < -50.0f || temperature > 150.0f)
        {
          ESP_LOGW(TAG, "Invalid temperature reading: %.2f°C (Voltage: %.3fV, Resistance: %.0fΩ)", temperature, voltage, resistance);
          temperature = -999.0f; // Mark as invalid
        }
        else
        {
          ESP_LOGD(TAG, "Calculated temperature: %.2f°C (Voltage: %.3fV, Resistance: %.0fΩ)", temperature, voltage, resistance);
        }

        // Create sample
        temp_sample_t sample = {
            .temperature = temperature,
            .voltage = voltage,
            .resistance = resistance,
            .timestamp = xTaskGetTickCount() * portTICK_PERIOD_MS};

        // Store in buffer
        circular_buffer_push(sensor->buffer, &sample);
      }

      // Small delay between sensors to avoid ADC conflicts
      if (i < params->sensor_count - 1)
      {
        vTaskDelay(pdMS_TO_TICKS(10));
      }
    }

    // Broadcast latest sensor readings to all WebSocket subscribers
    ws_broadcast_latest_sensor_data();

    // Wait for next reading interval
    vTaskDelay(pdMS_TO_TICKS(TEMP_READ_INTERVAL_MS));
  }
}

/**
 * @brief Calculate Steinhart-Hart coefficients from three temperature-resistance data points
 * @param p1 First calibration point
 * @param p2 Second calibration point
 * @param p3 Third calibration point
 * @return Steinhart-Hart coefficients structure
 */
steinhart_hart_coeffs_t calculate_steinhart_hart_coefficients(
    temperature_resistance_point_t p1,
    temperature_resistance_point_t p2,
    temperature_resistance_point_t p3)
{
  // Convert temperatures to Kelvin
  float tk1 = p1.temperature_celsius + 273.15f;
  float tk2 = p2.temperature_celsius + 273.15f;
  float tk3 = p3.temperature_celsius + 273.15f;

  // Calculate reciprocals
  float y1 = 1.0f / tk1;
  float y2 = 1.0f / tk2;
  float y3 = 1.0f / tk3;

  // Calculate natural logs of resistances
  float l1 = logf(p1.resistance_ohms);
  float l2 = logf(p2.resistance_ohms);
  float l3 = logf(p3.resistance_ohms);

  // Differences
  float d21 = l2 - l1;
  float d31 = l3 - l1;
  float dy21 = y2 - y1;
  float dy31 = y3 - y1;

  // Powers
  float p21 = l2 * l2 * l2 - l1 * l1 * l1; // (l2^3 - l1^3)
  float p31 = l3 * l3 * l3 - l1 * l1 * l1; // (l3^3 - l1^3)

  // Solve for C
  float denominator = p31 - p21 * d31 / d21;
  if (fabsf(denominator) < 1e-10f)
  {
    // Degenerate case, return invalid coefficients
    return (steinhart_hart_coeffs_t){0.0f, 0.0f, 0.0f};
  }

  float c = (dy31 - dy21 * d31 / d21) / denominator;

  // Solve for B
  float b = (dy21 - c * p21) / d21;

  // Solve for A
  float a = y1 - b * l1 - c * l1 * l1 * l1;

  return (steinhart_hart_coeffs_t){a, b, c};
}

/**
 * @brief Initialize ADC and temperature sampling system
 */
void temp_sensor_init(void)
{
  ESP_LOGI(TAG, "Initializing dual thermistor temperature sensors (Steinhart-Hart) ADC on GPIO1 and GPIO2");

  // ===== AIR TEMPERATURE SENSOR (ADC_CHANNEL_0) =====
  // Define three calibration points for air temperature sensor
  temperature_resistance_point_t air_cal_point_1 = {.temperature_celsius = AIR_TEMP_SAMPLE_1_CELSIUS, .resistance_ohms = AIR_TEMP_SAMPLE_1_OHMS};
  temperature_resistance_point_t air_cal_point_2 = {.temperature_celsius = AIR_TEMP_SAMPLE_2_CELSIUS, .resistance_ohms = AIR_TEMP_SAMPLE_2_OHMS};
  temperature_resistance_point_t air_cal_point_3 = {.temperature_celsius = AIR_TEMP_SAMPLE_3_CELSIUS, .resistance_ohms = AIR_TEMP_SAMPLE_3_OHMS};

  // Calculate Steinhart-Hart coefficients for air sensor
  steinhart_hart_coeffs_t air_coeffs = calculate_steinhart_hart_coefficients(air_cal_point_1, air_cal_point_2, air_cal_point_3);

  // Create air thermistor configuration
  static thermistor_config_t air_config = {
      .adc_channel = ADC_CHANNEL_0,
      .coeffs = {0.0f, 0.0f, 0.0f},                            // Will be assigned below
      .series_resistor = AIR_TEMP_SERIES_RESISTOR,             // 100kΩ series resistor
      .adc_voltage_reference = AIR_TEMP_ADC_VOLTAGE_REFERENCE, // 3.3V ADC reference
      .averaging_samples = TEMP_AVERAGE_SAMPLES};
  air_config.coeffs = air_coeffs; // Assign calculated coefficients

  ESP_LOGI(TAG, "Air sensor calibration: %.0f°C@%.0fΩ, %.0f°C@%.0fΩ, %.0f°C@%.0fΩ",
           air_cal_point_1.temperature_celsius, air_cal_point_1.resistance_ohms,
           air_cal_point_2.temperature_celsius, air_cal_point_2.resistance_ohms,
           air_cal_point_3.temperature_celsius, air_cal_point_3.resistance_ohms);
  ESP_LOGI(TAG, "Air coefficients: A=%.9f, B=%.9f, C=%.13f",
           air_coeffs.A, air_coeffs.B, air_coeffs.C);

  // ===== HEATER TEMPERATURE SENSOR (ADC_CHANNEL_1) =====
  // Define three calibration points for heater temperature sensor
  temperature_resistance_point_t heater_cal_point_1 = {.temperature_celsius = HEATER_TEMP_SAMPLE_1_CELSIUS, .resistance_ohms = HEATER_TEMP_SAMPLE_1_OHMS};
  temperature_resistance_point_t heater_cal_point_2 = {.temperature_celsius = HEATER_TEMP_SAMPLE_2_CELSIUS, .resistance_ohms = HEATER_TEMP_SAMPLE_2_OHMS};
  temperature_resistance_point_t heater_cal_point_3 = {.temperature_celsius = HEATER_TEMP_SAMPLE_3_CELSIUS, .resistance_ohms = HEATER_TEMP_SAMPLE_3_OHMS};

  // Calculate Steinhart-Hart coefficients for heater sensor
  steinhart_hart_coeffs_t heater_coeffs = calculate_steinhart_hart_coefficients(heater_cal_point_1, heater_cal_point_2, heater_cal_point_3);

  // Create heater thermistor configuration
  static thermistor_config_t heater_config = {
      .adc_channel = ADC_CHANNEL_1,
      .coeffs = {0.0f, 0.0f, 0.0f},                               // Will be assigned below
      .series_resistor = HEATER_TEMP_SERIES_RESISTOR,             // 100kΩ series resistor
      .adc_voltage_reference = HEATER_TEMP_ADC_VOLTAGE_REFERENCE, // 3.3V ADC reference
      .averaging_samples = TEMP_AVERAGE_SAMPLES};
  heater_config.coeffs = heater_coeffs; // Assign calculated coefficients

  // Set global config pointers for the dual task
  air_config_ptr = &air_config;
  heater_config_ptr = &heater_config;

  // Initialize sensor handles
  air_sensor_handle.buffer = &temp_buffer_1;
  air_sensor_handle.config = air_config_ptr;

  heater_sensor_handle.buffer = &temp_buffer_2;
  heater_sensor_handle.config = heater_config_ptr;

  ESP_LOGI(TAG, "Heater sensor calibration: %.0f°C@%.0fΩ, %.0f°C@%.0fΩ, %.0f°C@%.0fΩ",
           heater_cal_point_1.temperature_celsius, heater_cal_point_1.resistance_ohms,
           heater_cal_point_2.temperature_celsius, heater_cal_point_2.resistance_ohms,
           heater_cal_point_3.temperature_celsius, heater_cal_point_3.resistance_ohms);
  ESP_LOGI(TAG, "Heater coefficients: A=%.9f, B=%.9f, C=%.13f",
           heater_coeffs.A, heater_coeffs.B, heater_coeffs.C);

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
      .atten = ADC_ATTEN_DB_6,
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

  // Configure ADC1 channels for both sensors
  adc_oneshot_chan_cfg_t config = {
      .atten = ADC_ATTEN_DB_6, // 0-2.2V range for better resolution
      .bitwidth = ADC_BITWIDTH_12,
  };

  // Configure air sensor (ADC_CHANNEL_0)
  ret = adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_0, &config);
  if (ret != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to configure ADC channel 0: %s", esp_err_to_name(ret));
    adc_oneshot_del_unit(adc1_handle);
    adc1_handle = NULL;
    return;
  }

  // Configure heater sensor (ADC_CHANNEL_1)
  ret = adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_1, &config);
  if (ret != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to configure ADC channel 1: %s", esp_err_to_name(ret));
    adc_oneshot_del_unit(adc1_handle);
    adc1_handle = NULL;
    return;
  }

  // Initialize temperature buffers
  if (!circular_buffer_init(&temp_buffer_1, sizeof(temp_sample_t), TEMP_BUFFER_SIZE))
  {
    ESP_LOGE(TAG, "Failed to initialize air temperature buffer");
    return;
  }

  if (!circular_buffer_init(&temp_buffer_2, sizeof(temp_sample_t), TEMP_BUFFER_SIZE))
  {
    ESP_LOGE(TAG, "Failed to initialize heater temperature buffer");
    circular_buffer_free(&temp_buffer_1);
    return;
  }

  // Create sensor array for the task
  static sensor_info_t sensor_array[] = {
      {.config = &air_config, .buffer = &temp_buffer_1},   // Air sensor
      {.config = &heater_config, .buffer = &temp_buffer_2} // Heater sensor
  };

  // Create task parameters
  static temp_task_params_t task_params = {
      .sensors = sensor_array,
      .sensor_count = sizeof(sensor_array) / sizeof(sensor_info_t)};

  // Create multi-sensor temperature reading task
  BaseType_t result = sysmon_xTaskCreate(
      temp_task,
      "temp_task",
      TEMP_TASK_STACK_SIZE,
      &task_params, // Pass sensor array
      TEMP_TASK_PRIORITY,
      &temp_task_handle);

  if (result != pdPASS)
  {
    ESP_LOGE(TAG, "Failed to create dual temperature task");
    circular_buffer_free(&temp_buffer_1);
    circular_buffer_free(&temp_buffer_2);
    return;
  }

  ESP_LOGI(TAG, "Dual temperature sensors initialized with %d sample buffers in PSRAM", TEMP_BUFFER_SIZE);
}

/**
 * @brief Get handle to the air temperature sensor
 * @return Handle to the air temperature sensor, or NULL if not initialized
 */
temp_sensor_handle_t temp_sensor_get_air_sensor(void)
{
  if (circular_buffer_count(&temp_buffer_1) == 0)
  {
    return NULL; // Not initialized
  }

  return &air_sensor_handle;
}

/**
 * @brief Get handle to the heater temperature sensor
 * @return Handle to the heater temperature sensor, or NULL if not initialized
 */
temp_sensor_handle_t temp_sensor_get_heater_sensor(void)
{
  if (circular_buffer_count(&temp_buffer_2) == 0)
  {
    return NULL; // Not initialized
  }

  return &heater_sensor_handle;
}

/**
 * @brief Get the most recent temperature reading from a sensor
 * @param sensor Handle to the temperature sensor
 * @return Latest temperature in Celsius, or -999.0f if no samples available or invalid sensor
 */
float temp_sensor_get_reading(temp_sensor_handle_t sensor)
{
  if (sensor == NULL || sensor->buffer == NULL)
  {
    return -999.0f;
  }

  temp_sample_t sample;
  if (circular_buffer_get_latest(sensor->buffer, &sample))
  {
    return sample.temperature;
  }
  return -999.0f;
}

/**
 * @brief Get temperature sample at specific index from a sensor (0 = oldest, buffer_count-1 = newest)
 * @param sensor Handle to the temperature sensor
 * @param index Sample index (0 to buffer_count-1)
 * @param[out] sample Pointer to temp_sample_t to fill with sample data
 * @return true if sample was retrieved successfully, false otherwise
 */
bool temp_sensor_get_sample(temp_sensor_handle_t sensor, size_t index, temp_sample_t *sample)
{
  if (sensor == NULL || sensor->buffer == NULL || sample == NULL)
  {
    return false;
  }

  return circular_buffer_get_at_index(sensor->buffer, index, sample);
}

/**
 * @brief Get the most recent complete temperature sample from a sensor
 * @param sensor Handle to the temperature sensor
 * @param[out] sample Pointer to temp_sample_t to fill with sample data
 * @return true if sample was retrieved successfully, false otherwise
 */
bool temp_sensor_get_latest_sample(temp_sensor_handle_t sensor, temp_sample_t *sample)
{
  if (sensor == NULL || sensor->buffer == NULL || sample == NULL)
  {
    return false;
  }

  return circular_buffer_get_latest(sensor->buffer, sample);
}

/**
 * @brief Get number of stored temperature samples from a sensor
 * @param sensor Handle to the temperature sensor
 * @return Number of valid samples (0 to TEMP_BUFFER_SIZE), or 0 if invalid sensor
 */
size_t temp_sensor_get_sample_count(temp_sensor_handle_t sensor)
{
  if (sensor == NULL || sensor->buffer == NULL)
  {
    return 0;
  }

  return circular_buffer_count(sensor->buffer);
}

/**
 * @brief Get the most recent ADC voltage reading from a sensor (calibrated if available)
 * @param sensor Handle to the temperature sensor
 * @return Latest voltage in volts, or -999.0f if no samples available or invalid sensor
 */
float temp_sensor_get_voltage(temp_sensor_handle_t sensor)
{
  if (sensor == NULL || sensor->buffer == NULL)
  {
    return -999.0f;
  }

  temp_sample_t sample;
  if (circular_buffer_get_latest(sensor->buffer, &sample))
  {
    return sample.voltage;
  }
  return -999.0f;
}

/**
 * @brief Get the most recent thermistor resistance reading from a sensor
 * @param sensor Handle to the temperature sensor
 * @return Latest resistance in ohms, or -999.0f if no samples available or invalid sensor
 */
float temp_sensor_get_resistance(temp_sensor_handle_t sensor)
{
  if (sensor == NULL || sensor->buffer == NULL)
  {
    return -999.0f;
  }

  temp_sample_t sample;
  if (circular_buffer_get_latest(sensor->buffer, &sample))
  {
    return sample.resistance;
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
  circular_buffer_free(&temp_buffer_2);

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

/**
 * @brief Get pointer to air temperature circular buffer (for web server)
 * @return Pointer to air temperature buffer, or NULL if not initialized
 */
circular_buffer_t *get_air_temp_buffer(void)
{
  return circular_buffer_count(&temp_buffer_1) > 0 ? &temp_buffer_1 : NULL;
}

/**
 * @brief Get pointer to heater temperature circular buffer (for web server)
 * @return Pointer to heater temperature buffer, or NULL if not initialized
 */
circular_buffer_t *get_heater_temp_buffer(void)
{
  return circular_buffer_count(&temp_buffer_2) > 0 ? &temp_buffer_2 : NULL;
}

// Note: ADC averaging buffer is freed by the task itself when it exits
