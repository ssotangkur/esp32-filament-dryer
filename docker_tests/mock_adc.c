#include "mock_adc.h"
#include <stdlib.h>
#include <time.h>
#include <math.h>

// Include ESP-IDF ADC headers to override their functions
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"

// Global mock ADC configurations
mock_adc_config_t mock_adc_configs[ADC_CHANNEL_MAX];

// Internal state for intermittent failures
static uint32_t mock_start_time = 0;

// Forward declarations for ESP-IDF functions we're overriding
esp_err_t adc_oneshot_read(adc_oneshot_unit_handle_t handle, adc_channel_t chan, int *out_raw);
esp_err_t adc_cali_raw_to_voltage(adc_cali_handle_t handle, int adc_raw, int *voltage);

/**
 * @brief Initialize mock ADC system
 */
void mock_adc_init(void)
{
  // Seed random number generator
  srand((unsigned int)time(NULL));
  mock_start_time = 0; // Will be set on first intermittent call

  // Initialize all channels to normal operation
  for (int i = 0; i < ADC_CHANNEL_MAX; i++)
  {
    mock_adc_configs[i].mode = MOCK_ADC_MODE_NORMAL;
    mock_adc_configs[i].normal_value = 2048;        // Mid-range value
    mock_adc_configs[i].disconnect_threshold = 50;  // Low threshold for disconnection
    mock_adc_configs[i].noise_amplitude = 0.1f;     // 10% noise by default
    mock_adc_configs[i].intermittent_period = 1000; // 1 second period
  }
}

/**
 * @brief Set mock mode for a specific ADC channel
 */
void mock_adc_set_mode(adc_channel_t channel, mock_adc_mode_t mode)
{
  if (channel < ADC_CHANNEL_MAX)
  {
    mock_adc_configs[channel].mode = mode;
  }
}

/**
 * @brief Set normal operating value for a channel
 */
void mock_adc_set_normal_value(adc_channel_t channel, uint16_t value)
{
  if (channel < ADC_CHANNEL_MAX)
  {
    mock_adc_configs[channel].normal_value = value;
  }
}

/**
 * @brief Set disconnection threshold for a channel
 */
void mock_adc_set_disconnect_threshold(adc_channel_t channel, uint16_t threshold)
{
  if (channel < ADC_CHANNEL_MAX)
  {
    mock_adc_configs[channel].disconnect_threshold = threshold;
  }
}

/**
 * @brief Set noise amplitude for a channel (0.0-1.0)
 */
void mock_adc_set_noise_amplitude(adc_channel_t channel, float amplitude)
{
  if (channel < ADC_CHANNEL_MAX)
  {
    mock_adc_configs[channel].noise_amplitude = amplitude;
  }
}

/**
 * @brief Reset channel to normal operation
 */
void mock_adc_reset_to_normal(adc_channel_t channel)
{
  mock_adc_set_mode(channel, MOCK_ADC_MODE_NORMAL);
}

/**
 * @brief Simulate disconnected analog pin
 */
void mock_adc_simulate_disconnection(adc_channel_t channel)
{
  mock_adc_set_mode(channel, MOCK_ADC_MODE_DISCONNECTED);
}

/**
 * @brief Simulate short circuit condition
 */
void mock_adc_simulate_short_circuit(adc_channel_t channel)
{
  mock_adc_set_mode(channel, MOCK_ADC_MODE_SHORT_CIRCUIT);
}

/**
 * @brief Simulate noisy ADC readings
 */
void mock_adc_simulate_noise(adc_channel_t channel, float amplitude)
{
  mock_adc_set_noise_amplitude(channel, amplitude);
  mock_adc_set_mode(channel, MOCK_ADC_MODE_NOISE);
}

/**
 * @brief Simulate intermittent connection failures
 */
void mock_adc_simulate_intermittent(adc_channel_t channel, uint32_t period_ms)
{
  if (channel < ADC_CHANNEL_MAX)
  {
    mock_adc_configs[channel].intermittent_period = period_ms;
    mock_adc_set_mode(channel, MOCK_ADC_MODE_INTERMITTENT);
  }
}

/**
 * @brief Generate mock ADC reading based on current mode
 */
static int generate_mock_reading(adc_channel_t channel)
{
  mock_adc_config_t *config = &mock_adc_configs[channel];
  int base_value = config->normal_value;

  switch (config->mode)
  {
  case MOCK_ADC_MODE_NORMAL:
    // Normal operation with optional noise
    if (config->noise_amplitude > 0.0f)
    {
      float noise = ((float)rand() / RAND_MAX - 0.5f) * 2.0f * config->noise_amplitude;
      base_value = (int)(base_value * (1.0f + noise));
      // Clamp to valid ADC range
      if (base_value < 0)
        base_value = 0;
      if (base_value > 4095)
        base_value = 4095;
    }
    return base_value;

  case MOCK_ADC_MODE_DISCONNECTED:
    // Disconnected pins typically read very low values
    return rand() % config->disconnect_threshold;

  case MOCK_ADC_MODE_SHORT_CIRCUIT:
    // Short circuits typically read maximum values
    return 4095 - (rand() % 10); // Slight variation around max

  case MOCK_ADC_MODE_NOISE:
    // Pure noise - random values across full range
    return rand() % 4096;

  case MOCK_ADC_MODE_FLOATING:
    // Floating pins can read any value unpredictably
    return rand() % 4096;

  case MOCK_ADC_MODE_INTERMITTENT:
    // Alternate between normal and disconnected
    if (mock_start_time == 0)
    {
      mock_start_time = (uint32_t)time(NULL) * 1000; // Simple time simulation
    }
    uint32_t current_time = (uint32_t)time(NULL) * 1000;
    uint32_t elapsed = current_time - mock_start_time;
    bool is_connected = (elapsed / config->intermittent_period) % 2 == 0;

    if (is_connected)
    {
      return config->normal_value + (rand() % 100 - 50); // Normal with small variation
    }
    else
    {
      return rand() % config->disconnect_threshold; // Disconnected
    }

  default:
    return config->normal_value;
  }
}

// Mock implementations of ESP-IDF ADC functions

/**
 * @brief Mock implementation of adc_oneshot_read
 * This overrides the ESP-IDF function to return mock values
 */
esp_err_t adc_oneshot_read(adc_oneshot_unit_handle_t handle, adc_channel_t chan, int *out_raw)
{
  if (out_raw == NULL || chan >= ADC_CHANNEL_MAX)
  {
    return ESP_ERR_INVALID_ARG;
  }

  *out_raw = generate_mock_reading(chan);
  return ESP_OK;
}

/**
 * @brief Mock implementation of adc_cali_raw_to_voltage
 * Converts mock ADC readings to voltage (simplified calibration)
 */
esp_err_t adc_cali_raw_to_voltage(adc_cali_handle_t handle, int adc_raw, int *voltage)
{
  if (voltage == NULL)
  {
    return ESP_ERR_INVALID_ARG;
  }

  // Simple linear conversion: 0-4095 -> 0-3300mV
  *voltage = (adc_raw * 3300) / 4095;
  return ESP_OK;
}
