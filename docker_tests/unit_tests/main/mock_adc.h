#ifndef MOCK_ADC_H
#define MOCK_ADC_H

#include <stdint.h>
#include <stdbool.h>

// Mock ADC behavior modes for testing
typedef enum
{
  MOCK_ADC_MODE_NORMAL,        // Normal operation
  MOCK_ADC_MODE_DISCONNECTED,  // Pin disconnected (reads 0 or very low)
  MOCK_ADC_MODE_SHORT_CIRCUIT, // Short circuit (reads max value)
  MOCK_ADC_MODE_NOISE,         // Random noise
  MOCK_ADC_MODE_FLOATING,      // Floating pin (unpredictable values)
  MOCK_ADC_MODE_INTERMITTENT   // Intermittent connection
} mock_adc_mode_t;

// Mock ADC channel configuration
typedef struct
{
  mock_adc_mode_t mode;
  uint16_t normal_value;         // Value when operating normally
  uint16_t disconnect_threshold; // Threshold for disconnected detection
  float noise_amplitude;         // Noise magnitude (0.0-1.0)
  uint32_t intermittent_period;  // Period for intermittent failures (ms)
} mock_adc_config_t;

// ADC channel constants (from ESP-IDF)
#define ADC_CHANNEL_MAX 10 // Maximum ADC channels supported

// Global mock ADC state
extern mock_adc_config_t mock_adc_configs[ADC_CHANNEL_MAX];

// Mock ADC control functions
void mock_adc_init(void);
void mock_adc_set_mode(adc_channel_t channel, mock_adc_mode_t mode);
void mock_adc_set_normal_value(adc_channel_t channel, uint16_t value);
void mock_adc_set_disconnect_threshold(adc_channel_t channel, uint16_t threshold);
void mock_adc_set_noise_amplitude(adc_channel_t channel, float amplitude);
void mock_adc_reset_to_normal(adc_channel_t channel);

// Test scenario helpers
void mock_adc_simulate_disconnection(adc_channel_t channel);
void mock_adc_simulate_short_circuit(adc_channel_t channel);
void mock_adc_simulate_noise(adc_channel_t channel, float amplitude);
void mock_adc_simulate_intermittent(adc_channel_t channel, uint32_t period_ms);

#endif // MOCK_ADC_H
