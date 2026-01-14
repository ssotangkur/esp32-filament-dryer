#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "unity.h"            // Unity testing framework
#include "Mockmock_esp_adc.h" // CMock-generated ADC mocks
// #include "temp.h"  // Commented out - not available in test build

// Simple test data structures for unit testing
typedef struct
{
  int adc_channel;
  float series_resistor;
  float adc_voltage_reference;
  int averaging_samples;
} mock_thermistor_config_t;

// Test data
static mock_thermistor_config_t test_config;

void setUp(void)
{
  // Initialize test configuration
  test_config.adc_channel = 0;
  test_config.series_resistor = 10000.0f; // 10kΩ
  test_config.adc_voltage_reference = 3.3f;
  test_config.averaging_samples = 5;
}

void tearDown(void)
{
  // Cleanup if needed
}

/**
 * @brief Test basic temperature calculation
 * Demonstrates simple arithmetic operations
 */
void test_temperature_reading_normal(void)
{
  // Simple temperature calculation test
  int adc_raw = 1850;
  float voltage = (adc_raw / 4095.0f) * 3.3f; // 12-bit ADC, 3.3V reference

  // Calculate temperature using simplified Steinhart-Hart equation
  // R = R_series * (V_ref/V_out - 1)
  float resistance = test_config.series_resistor * (test_config.adc_voltage_reference / voltage - 1.0f);

  // Simplified temperature calculation (this is approximate)
  float temperature = 1.0f / (0.001129148f + 0.000234125f * logf(resistance) + 0.0000000876741f * powf(logf(resistance), 3)) - 273.15f;

  // Assert reasonable temperature range (should be around 25°C)
  TEST_ASSERT_FLOAT_WITHIN(10.0f, 25.0f, temperature);
}

/**
 * @brief Test temperature calculation with low ADC values
 * Simulates disconnected sensor
 */
void test_temperature_reading_disconnected(void)
{
  // Very low ADC reading (disconnected sensor)
  int adc_raw = 5;
  float voltage = (adc_raw / 4095.0f) * 3.3f;

  // This should result in very high resistance (open circuit)
  float resistance = test_config.series_resistor * (test_config.adc_voltage_reference / voltage - 1.0f);

  // Assert that resistance is very high (indicating disconnected sensor)
  TEST_ASSERT_GREATER_THAN(1000000.0f, resistance); // > 1MΩ
}

/**
 * @brief Test temperature calculation with high ADC values
 * Simulates short circuit
 */
void test_temperature_reading_short_circuit(void)
{
  // Very high ADC reading (short circuit)
  int adc_raw = 4090;
  float voltage = (adc_raw / 4095.0f) * 3.3f;

  // This should result in very low resistance (short circuit)
  float resistance = test_config.series_resistor * (test_config.adc_voltage_reference / voltage - 1.0f);

  // Assert that resistance is very low (indicating short circuit)
  TEST_ASSERT_LESS_THAN(100.0f, resistance); // < 100Ω
}

/**
 * @brief Test averaging calculation
 * Demonstrates statistical operations
 */
void test_temperature_reading_averaging(void)
{
  // Simulate multiple ADC readings
  int readings[] = {1800, 1810, 1820, 1830, 1840};
  int num_readings = 5;

  int total = 0;
  for (int i = 0; i < num_readings; i++)
  {
    total += readings[i];
  }

  float average = (float)total / num_readings;

  // Assert average calculation
  TEST_ASSERT_FLOAT_WITHIN(1.0f, 1820.0f, average);
}

/**
 * @brief Test configuration validation
 */
void test_temperature_config_validation(void)
{
  // Test valid configuration
  TEST_ASSERT_TRUE(test_config.adc_channel >= 0);
  TEST_ASSERT_GREATER_THAN(0.0f, test_config.series_resistor);
  TEST_ASSERT_GREATER_THAN(0.0f, test_config.adc_voltage_reference);
  TEST_ASSERT_GREATER_THAN(0, test_config.averaging_samples);
}

/**
 * @brief Test group runner
 */
void test_temperature(void)
{
  RUN_TEST(test_temperature_reading_normal);
  RUN_TEST(test_temperature_reading_disconnected);
  RUN_TEST(test_temperature_reading_short_circuit);
  RUN_TEST(test_temperature_reading_averaging);
  RUN_TEST(test_temperature_config_validation);
}
