#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include "unity.h"

// Include temperature sensor header for types and function declarations
#include "../../include/temp.h"

// Include CMock-generated mock headers for ESP-IDF functions
#include "Mockmock_semphr.h"
#include "Mockmock_esp_adc.h"
#include "Mockmock_esp_heap_caps.h"
#include "Mockmock_sysmon_wrapper.h"

// Declare the function from temp.c (we'll include temp.c in the build but not in this header)
extern steinhart_hart_coeffs_t calculate_steinhart_hart_coefficients(
    temperature_resistance_point_t p1,
    temperature_resistance_point_t p2,
    temperature_resistance_point_t p3);

// Declare temp_sensor_init function for testing
extern void temp_sensor_init(void);

/**
 * @brief Test Steinhart-Hart coefficient calculation
 */
void test_calculate_steinhart_hart_coefficients(void)
{
  // Test with the same calibration points used in temp_sensor_init
  temperature_resistance_point_t p1 = {25.0f, 988000.0f};
  temperature_resistance_point_t p2 = {50.0f, 35230.0f};
  temperature_resistance_point_t p3 = {85.0f, 10560.0f};

  steinhart_hart_coeffs_t coeffs = calculate_steinhart_hart_coefficients(p1, p2, p3);

  // Verify coefficients are reasonable (finite, non-zero values)
  TEST_ASSERT_TRUE(isfinite(coeffs.A));
  TEST_ASSERT_TRUE(isfinite(coeffs.B));
  TEST_ASSERT_TRUE(isfinite(coeffs.C));
  TEST_ASSERT_TRUE(coeffs.A != 0.0f);
  TEST_ASSERT_TRUE(coeffs.B != 0.0f);
  TEST_ASSERT_TRUE(coeffs.C != 0.0f);
}

/**
 * @brief Test temperature sensor initialization
 *
 * This test demonstrates that temp_sensor_init() can be successfully mocked and tested.
 * We mock all the ESP-IDF functions it calls and verify that the initialization completes
 * and sensors become available.
 */
void test_temp_sensor_init(void)
{
  // Initialize CMock mocks
  Mockmock_semphr_Init();
  Mockmock_esp_adc_Init();
  Mockmock_esp_heap_caps_Init();

  // Mock all ESP-IDF functions called by temp_sensor_init()
  // These mocks allow the function to run without actual hardware dependencies

  // Mock semaphore creation (called twice for sensor mutexes)
  xSemaphoreCreateMutex_ExpectAndReturn((SemaphoreHandle_t)0x1000);
  xSemaphoreCreateMutex_ExpectAndReturn((SemaphoreHandle_t)0x2000);

  // Mock ADC initialization functions - ignore arguments and return success
  adc_oneshot_new_unit_IgnoreAndReturn(ESP_OK);
  adc_cali_create_scheme_curve_fitting_IgnoreAndReturn(ESP_OK);
  adc_oneshot_config_channel_IgnoreAndReturn(ESP_OK);

  // Mock task creation
  sysmon_xTaskCreate_IgnoreAndReturn(pdPASS);

  // Call the function under test
  temp_sensor_init();

  // Verify successful initialization by checking that sensors are now available
  temp_sensor_handle_t air_sensor = temp_sensor_get_air_sensor();
  temp_sensor_handle_t heater_sensor = temp_sensor_get_heater_sensor();

  // The key assertion: sensors should be available after successful initialization
  TEST_ASSERT_NOT_NULL(air_sensor);
  TEST_ASSERT_NOT_NULL(heater_sensor);

  // Additional verification that the sensors are properly initialized
  TEST_ASSERT_EQUAL(0, temp_sensor_get_sample_count(air_sensor));
  TEST_ASSERT_EQUAL(0, temp_sensor_get_sample_count(heater_sensor));
  TEST_ASSERT_EQUAL(-999.0f, temp_sensor_get_reading(air_sensor));
  TEST_ASSERT_EQUAL(-999.0f, temp_sensor_get_reading(heater_sensor));
}

/**
 * @brief Test group runner
 */
void test_temp(void)
{
  printf("Running temperature sensor tests...\n");
  RUN_TEST(test_calculate_steinhart_hart_coefficients);
  RUN_TEST(test_temp_sensor_init);
  printf("Temperature sensor tests completed\n");
}
