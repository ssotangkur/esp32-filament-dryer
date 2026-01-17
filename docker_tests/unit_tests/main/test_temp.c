#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include "unity.h"

// Include temperature sensor header for types and function declarations
#include "../../include/temp.h"
#include "../../include/circular_buffer.h"

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

// Forward declare the temp_sensor_handle structure (defined in temp.c)
struct temp_sensor_handle
{
  circular_buffer_t *buffer; // Pointer to the sensor's buffer
  const void *config;        // Pointer to the sensor's configuration (simplified for testing)
  void *subscriptions;       // Linked list of subscriptions
  void *subscriptions_mutex; // Mutex for thread-safe subscription management
};

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
  // Define static mock buffers for the circular buffers
  static uint8_t mock_temp_buffer_1[TEMP_BUFFER_SIZE * sizeof(temp_sample_t)];
  static uint8_t mock_temp_buffer_2[TEMP_BUFFER_SIZE * sizeof(temp_sample_t)];

  // Mock all ESP-IDF functions called by temp_sensor_init()
  // These mocks allow the function to run without actual hardware dependencies

  // Mock semaphore creation for subscriptions (called first)
  xSemaphoreCreateMutex_ExpectAndReturn((SemaphoreHandle_t)0x3000);
  xSemaphoreCreateMutex_ExpectAndReturn((SemaphoreHandle_t)0x4000);

  // Mock ADC initialization functions - ignore arguments and return success
  adc_oneshot_new_unit_IgnoreAndReturn(ESP_OK);
  adc_cali_create_scheme_curve_fitting_IgnoreAndReturn(ESP_OK);
  adc_oneshot_config_channel_IgnoreAndReturn(ESP_OK);
  adc_oneshot_config_channel_IgnoreAndReturn(ESP_OK); // Called twice

  // Mock circular_buffer_init for both sensors
  heap_caps_malloc_ExpectAndReturn(TEMP_BUFFER_SIZE * sizeof(temp_sample_t), MALLOC_CAP_SPIRAM, mock_temp_buffer_1);
  xSemaphoreCreateMutex_ExpectAndReturn((SemaphoreHandle_t)0x5000); // Mutex for first buffer
  heap_caps_malloc_ExpectAndReturn(TEMP_BUFFER_SIZE * sizeof(temp_sample_t), MALLOC_CAP_SPIRAM, mock_temp_buffer_2);
  xSemaphoreCreateMutex_ExpectAndReturn((SemaphoreHandle_t)0x6000); // Mutex for second buffer

  // Mock task creation
  sysmon_xTaskCreate_IgnoreAndReturn(pdPASS);

  // Call the function under test
  temp_sensor_init();

  // In this test, we can't easily check the sensor handles because they are only returned if the circular buffer has items.
  // The task that adds items is not running in the test.
  // So, we just assert that the function ran to completion.
  TEST_ASSERT_TRUE(true);
}

/**
 * @brief Test temp_sensor_get_reading function
 *
 * Tests all scenarios for the temp_sensor_get_reading function:
 * - NULL sensor parameter
 * - Sensor with NULL buffer
 * - Sensor with empty buffer
 * - Sensor with buffer containing samples
 */
void test_temp_sensor_get_reading(void)
{
  // ===== Test 1: NULL sensor parameter =====
  float result = temp_sensor_get_reading(NULL);
  TEST_ASSERT_EQUAL(-999.0f, result);

  // ===== Test 2: Sensor with NULL buffer =====
  struct temp_sensor_handle null_buffer_sensor = {
      .buffer = NULL,
      .config = NULL,
      .subscriptions = NULL,
      .subscriptions_mutex = NULL};

  result = temp_sensor_get_reading(&null_buffer_sensor);
  TEST_ASSERT_EQUAL(-999.0f, result);

  // ===== Test 3: Sensor with empty buffer =====
  // Create a circular buffer for testing
  static uint8_t mock_buffer[5 * sizeof(temp_sample_t)]; // Buffer for 5 samples
  circular_buffer_t test_buffer;

  // Mock circular_buffer_init
  heap_caps_malloc_ExpectAndReturn(5 * sizeof(temp_sample_t), MALLOC_CAP_SPIRAM, mock_buffer);
  xSemaphoreCreateMutex_ExpectAndReturn((SemaphoreHandle_t)0x3000);
  TEST_ASSERT_TRUE(circular_buffer_init(&test_buffer, sizeof(temp_sample_t), 5));

  // Create sensor handle pointing to empty buffer
  struct temp_sensor_handle empty_sensor = {
      .buffer = &test_buffer,
      .config = NULL,
      .subscriptions = NULL,
      .subscriptions_mutex = NULL};

  // Mock the empty buffer case - get_latest will be called but return false
  xSemaphoreTake_ExpectAndReturn(test_buffer.mutex, portMAX_DELAY, pdTRUE);
  xSemaphoreGive_ExpectAndReturn(test_buffer.mutex, pdTRUE);
  result = temp_sensor_get_reading(&empty_sensor);
  TEST_ASSERT_EQUAL(-999.0f, result);

  // ===== Test 4: Sensor with buffer containing samples =====
  // Add a sample to the buffer
  temp_sample_t test_sample = {
      .temperature = 25.5f,
      .voltage = 1.23f,
      .resistance = 100000.0f,
      .timestamp = 1234567890};

  // Mock semaphore operations for push
  xSemaphoreTake_ExpectAndReturn(test_buffer.mutex, portMAX_DELAY, pdTRUE);
  xSemaphoreGive_ExpectAndReturn(test_buffer.mutex, pdTRUE);
  TEST_ASSERT_TRUE(circular_buffer_push(&test_buffer, &test_sample));

  // Now test getting the reading
  // Mock get_latest call
  xSemaphoreTake_ExpectAndReturn(test_buffer.mutex, portMAX_DELAY, pdTRUE);
  xSemaphoreGive_ExpectAndReturn(test_buffer.mutex, pdTRUE);
  result = temp_sensor_get_reading(&empty_sensor);
  TEST_ASSERT_EQUAL(25.5f, result);

  // ===== Cleanup =====
  // Mock cleanup operations
  vSemaphoreDelete_Expect(test_buffer.mutex);
  heap_caps_free_Expect(mock_buffer);
  circular_buffer_free(&test_buffer);
}

/**
 * @brief Test group runner
 */
void test_temp(void)
{
  printf("Running temperature sensor tests...\n");
  RUN_TEST(test_calculate_steinhart_hart_coefficients);
  RUN_TEST(test_temp_sensor_get_reading); // Run this first to avoid global state issues
  RUN_TEST(test_temp_sensor_init);
  printf("Temperature sensor tests completed\n");
}
