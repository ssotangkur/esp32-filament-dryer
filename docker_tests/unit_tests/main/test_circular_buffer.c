#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "unity.h"

// Include CMock-generated mock headers
#include "Mockmock_semphr.h"
#include "Mockmock_esp_heap_caps.h"

// Include the real circular buffer implementation
#include "../../main/circular_buffer.c"

// Test data structure
typedef struct
{
  int value;
  char name[32];
} test_item_t;

// Test buffer
static circular_buffer_t test_buffer;

// Mock memory buffers for testing
static uint8_t mock_buffer_10[10 * sizeof(test_item_t)]; // For tests using buffer size 10

// Helper function to initialize test buffer
void init_test_buffer(void)
{
  circular_buffer_init(&test_buffer, sizeof(test_item_t), 10);
}

// Helper function to cleanup test buffer
void cleanup_test_buffer(void)
{
  circular_buffer_free(&test_buffer);
}

// Test circular buffer initialization
void test_circular_buffer_init(void)
{
  circular_buffer_t test_buf;
  static uint8_t mock_buffer[5 * sizeof(test_item_t)]; // Real memory for testing

  // Initialize CMock mocks
  Mockmock_semphr_Init();
  Mockmock_esp_heap_caps_Init();

  // Set up mock expectations - return pointer to real allocated memory
  heap_caps_malloc_CMockExpectAndReturn(1, 5 * sizeof(test_item_t), MALLOC_CAP_SPIRAM, mock_buffer);
  xSemaphoreCreateMutex_CMockExpectAndReturn(2, (SemaphoreHandle_t)0x2000);

  // Mock expectations for semaphore operations during count/is_empty/is_full checks
  xSemaphoreTake_CMockExpectAndReturn(3, (SemaphoreHandle_t)0x2000, portMAX_DELAY, pdTRUE);
  xSemaphoreGive_CMockExpectAndReturn(4, (SemaphoreHandle_t)0x2000, pdTRUE);
  xSemaphoreTake_CMockExpectAndReturn(5, (SemaphoreHandle_t)0x2000, portMAX_DELAY, pdTRUE);
  xSemaphoreGive_CMockExpectAndReturn(6, (SemaphoreHandle_t)0x2000, pdTRUE);
  xSemaphoreTake_CMockExpectAndReturn(7, (SemaphoreHandle_t)0x2000, portMAX_DELAY, pdTRUE);
  xSemaphoreGive_CMockExpectAndReturn(8, (SemaphoreHandle_t)0x2000, pdTRUE);

  // Mock expectation for semaphore deletion during cleanup
  vSemaphoreDelete_CMockExpect(9, (SemaphoreHandle_t)0x2000);

  // Mock expectation for heap free during cleanup
  heap_caps_free_CMockExpect(10, mock_buffer);

  TEST_ASSERT_TRUE(circular_buffer_init(&test_buf, sizeof(test_item_t), 5));
  TEST_ASSERT_EQUAL(0, circular_buffer_count(&test_buf));
  TEST_ASSERT_EQUAL(5, test_buf.buffer_size);
  TEST_ASSERT_TRUE(circular_buffer_is_empty(&test_buf));
  TEST_ASSERT_FALSE(circular_buffer_is_full(&test_buf));

  // Clean up
  circular_buffer_free(&test_buf);
}

// Test initialization failure cases
void test_circular_buffer_init_failures(void)
{
  circular_buffer_t invalid_buffer;

  // NULL buffer
  TEST_ASSERT_FALSE(circular_buffer_init(NULL, sizeof(test_item_t), 5));

  // Zero element size
  TEST_ASSERT_FALSE(circular_buffer_init(&invalid_buffer, 0, 5));

  // Zero buffer size
  TEST_ASSERT_FALSE(circular_buffer_init(&invalid_buffer, sizeof(test_item_t), 0));
}

// Test basic push and pop operations
void test_circular_buffer_push_pop(void)
{
  circular_buffer_t test_buf;

  // Initialize CMock mocks
  Mockmock_semphr_Init();
  Mockmock_esp_heap_caps_Init();

  // Set up mock expectations for init
  heap_caps_malloc_CMockExpectAndReturn(1, 10 * sizeof(test_item_t), MALLOC_CAP_SPIRAM, mock_buffer_10);
  xSemaphoreCreateMutex_CMockExpectAndReturn(2, (SemaphoreHandle_t)0x2000);

  // Mock expectations for semaphore operations during operations
  xSemaphoreTake_CMockExpectAndReturn(3, (SemaphoreHandle_t)0x2000, portMAX_DELAY, pdTRUE);
  xSemaphoreGive_CMockExpectAndReturn(4, (SemaphoreHandle_t)0x2000, pdTRUE);
  xSemaphoreTake_CMockExpectAndReturn(5, (SemaphoreHandle_t)0x2000, portMAX_DELAY, pdTRUE);
  xSemaphoreGive_CMockExpectAndReturn(6, (SemaphoreHandle_t)0x2000, pdTRUE);
  xSemaphoreTake_CMockExpectAndReturn(7, (SemaphoreHandle_t)0x2000, portMAX_DELAY, pdTRUE);
  xSemaphoreGive_CMockExpectAndReturn(8, (SemaphoreHandle_t)0x2000, pdTRUE);
  xSemaphoreTake_CMockExpectAndReturn(9, (SemaphoreHandle_t)0x2000, portMAX_DELAY, pdTRUE);
  xSemaphoreGive_CMockExpectAndReturn(10, (SemaphoreHandle_t)0x2000, pdTRUE);
  xSemaphoreTake_CMockExpectAndReturn(11, (SemaphoreHandle_t)0x2000, portMAX_DELAY, pdTRUE);
  xSemaphoreGive_CMockExpectAndReturn(12, (SemaphoreHandle_t)0x2000, pdTRUE);

  // Mock expectation for semaphore deletion during cleanup
  vSemaphoreDelete_CMockExpect(13, (SemaphoreHandle_t)0x2000);

  // More mock expectations for semaphore operations during operations
  xSemaphoreTake_CMockExpectAndReturn(15, (SemaphoreHandle_t)0x2000, portMAX_DELAY, pdTRUE);
  xSemaphoreGive_CMockExpectAndReturn(16, (SemaphoreHandle_t)0x2000, pdTRUE);
  xSemaphoreTake_CMockExpectAndReturn(17, (SemaphoreHandle_t)0x2000, portMAX_DELAY, pdTRUE);
  xSemaphoreGive_CMockExpectAndReturn(18, (SemaphoreHandle_t)0x2000, pdTRUE);
  xSemaphoreTake_CMockExpectAndReturn(19, (SemaphoreHandle_t)0x2000, portMAX_DELAY, pdTRUE);
  xSemaphoreGive_CMockExpectAndReturn(20, (SemaphoreHandle_t)0x2000, pdTRUE);
  xSemaphoreTake_CMockExpectAndReturn(21, (SemaphoreHandle_t)0x2000, portMAX_DELAY, pdTRUE);
  xSemaphoreGive_CMockExpectAndReturn(22, (SemaphoreHandle_t)0x2000, pdTRUE);

  // Mock expectation for heap free during cleanup
  heap_caps_free_CMockExpect(23, mock_buffer_10);

  // Initialize buffer
  TEST_ASSERT_TRUE(circular_buffer_init(&test_buf, sizeof(test_item_t), 10));

  test_item_t item1 = {42, "test_item_1"};
  test_item_t item2 = {84, "test_item_2"};
  test_item_t result;

  // Push items
  TEST_ASSERT_TRUE(circular_buffer_push(&test_buf, &item1));
  TEST_ASSERT_EQUAL(1, circular_buffer_count(&test_buf));
  TEST_ASSERT_FALSE(circular_buffer_is_empty(&test_buf));

  TEST_ASSERT_TRUE(circular_buffer_push(&test_buf, &item2));
  TEST_ASSERT_EQUAL(2, circular_buffer_count(&test_buf));

  // Get latest (should be item2)
  TEST_ASSERT_TRUE(circular_buffer_get_latest(&test_buf, &result));
  TEST_ASSERT_EQUAL(84, result.value);
  TEST_ASSERT_EQUAL(0, strcmp("test_item_2", result.name));
  TEST_ASSERT_EQUAL(2, circular_buffer_count(&test_buf)); // get_latest doesn't remove

  // Get at index 0 (should be item1)
  TEST_ASSERT_TRUE(circular_buffer_get_at_index(&test_buf, 0, &result));
  TEST_ASSERT_EQUAL(42, result.value);
  TEST_ASSERT_EQUAL(0, strcmp("test_item_1", result.name));

  // Clean up
  circular_buffer_free(&test_buf);
}

// Test buffer overflow behavior (buffer accepts unlimited pushes)
void test_circular_buffer_overflow(void)
{
  circular_buffer_t test_buf;

  // Initialize CMock mocks
  Mockmock_semphr_Init();
  Mockmock_esp_heap_caps_Init();

  // Set up mock expectations for init
  heap_caps_malloc_CMockExpectAndReturn(1, 10 * sizeof(test_item_t), MALLOC_CAP_SPIRAM, mock_buffer_10);
  xSemaphoreCreateMutex_CMockExpectAndReturn(2, (SemaphoreHandle_t)0x2000);

  // Mock expectations for semaphore operations - simplified
  for (int i = 3; i <= 50; i += 2)
  {
    xSemaphoreTake_CMockExpectAndReturn(i, (SemaphoreHandle_t)0x2000, portMAX_DELAY, pdTRUE);
    xSemaphoreGive_CMockExpectAndReturn(i + 1, (SemaphoreHandle_t)0x2000, pdTRUE);
  }

  // Mock expectation for semaphore deletion during cleanup
  vSemaphoreDelete_CMockExpect(51, (SemaphoreHandle_t)0x2000);
  heap_caps_free_CMockExpect(52, mock_buffer_10);

  // Initialize buffer
  TEST_ASSERT_TRUE(circular_buffer_init(&test_buf, sizeof(test_item_t), 10));

  test_item_t item;

  // Fill buffer to capacity
  for (int i = 0; i < test_buf.buffer_size; i++)
  {
    item.value = i;
    sprintf(item.name, "item_%d", i);
    TEST_ASSERT_TRUE(circular_buffer_push(&test_buf, &item));
  }

  TEST_ASSERT_EQUAL(test_buf.buffer_size, circular_buffer_count(&test_buf));

  // Push beyond capacity - real implementation allows this
  item.value = 999;
  sprintf(item.name, "overflow_item");
  TEST_ASSERT_TRUE(circular_buffer_push(&test_buf, &item)); // Should succeed

  // Count should stay at buffer size limit
  TEST_ASSERT_EQUAL(test_buf.buffer_size, circular_buffer_count(&test_buf));

  // Clean up
  circular_buffer_free(&test_buf);
}

// Test buffer empty condition
void test_circular_buffer_empty(void)
{
  circular_buffer_t test_buf;

  // Initialize CMock mocks
  Mockmock_semphr_Init();
  Mockmock_esp_heap_caps_Init();

  // Set up mock expectations for init
  heap_caps_malloc_CMockExpectAndReturn(1, 10 * sizeof(test_item_t), MALLOC_CAP_SPIRAM, mock_buffer_10);
  xSemaphoreCreateMutex_CMockExpectAndReturn(2, (SemaphoreHandle_t)0x2000);

  // Mock expectations for semaphore operations during operations
  xSemaphoreTake_CMockExpectAndReturn(3, (SemaphoreHandle_t)0x2000, portMAX_DELAY, pdTRUE);
  xSemaphoreGive_CMockExpectAndReturn(4, (SemaphoreHandle_t)0x2000, pdTRUE);
  xSemaphoreTake_CMockExpectAndReturn(5, (SemaphoreHandle_t)0x2000, portMAX_DELAY, pdTRUE);
  xSemaphoreGive_CMockExpectAndReturn(6, (SemaphoreHandle_t)0x2000, pdTRUE);
  xSemaphoreTake_CMockExpectAndReturn(7, (SemaphoreHandle_t)0x2000, portMAX_DELAY, pdTRUE);
  xSemaphoreGive_CMockExpectAndReturn(8, (SemaphoreHandle_t)0x2000, pdTRUE);

  // Mock expectation for semaphore deletion during cleanup
  vSemaphoreDelete_CMockExpect(9, (SemaphoreHandle_t)0x2000);

  // Mock expectation for heap free during cleanup
  heap_caps_free_CMockExpect(10, mock_buffer_10);

  // Initialize buffer
  TEST_ASSERT_TRUE(circular_buffer_init(&test_buf, sizeof(test_item_t), 10));

  test_item_t result;
  TEST_ASSERT_FALSE(circular_buffer_get_latest(&test_buf, &result));
  TEST_ASSERT_EQUAL(0, circular_buffer_count(&test_buf));
  TEST_ASSERT_TRUE(circular_buffer_is_empty(&test_buf));

  // Clean up
  circular_buffer_free(&test_buf);
}

// Test buffer clear operation
void test_circular_buffer_clear(void)
{
  circular_buffer_t test_buf;

  // Initialize CMock mocks
  Mockmock_semphr_Init();
  Mockmock_esp_heap_caps_Init();

  // Set up mock expectations for init
  heap_caps_malloc_CMockExpectAndReturn(1, 10 * sizeof(test_item_t), MALLOC_CAP_SPIRAM, mock_buffer_10);
  xSemaphoreCreateMutex_CMockExpectAndReturn(2, (SemaphoreHandle_t)0x2000);

  // Mock expectations for semaphore operations during operations
  xSemaphoreTake_CMockExpectAndReturn(3, (SemaphoreHandle_t)0x2000, portMAX_DELAY, pdTRUE);
  xSemaphoreGive_CMockExpectAndReturn(4, (SemaphoreHandle_t)0x2000, pdTRUE);
  xSemaphoreTake_CMockExpectAndReturn(5, (SemaphoreHandle_t)0x2000, portMAX_DELAY, pdTRUE);
  xSemaphoreGive_CMockExpectAndReturn(6, (SemaphoreHandle_t)0x2000, pdTRUE);
  xSemaphoreTake_CMockExpectAndReturn(7, (SemaphoreHandle_t)0x2000, portMAX_DELAY, pdTRUE);
  xSemaphoreGive_CMockExpectAndReturn(8, (SemaphoreHandle_t)0x2000, pdTRUE);
  xSemaphoreTake_CMockExpectAndReturn(9, (SemaphoreHandle_t)0x2000, portMAX_DELAY, pdTRUE);
  xSemaphoreGive_CMockExpectAndReturn(10, (SemaphoreHandle_t)0x2000, pdTRUE);
  xSemaphoreTake_CMockExpectAndReturn(11, (SemaphoreHandle_t)0x2000, portMAX_DELAY, pdTRUE);
  xSemaphoreGive_CMockExpectAndReturn(12, (SemaphoreHandle_t)0x2000, pdTRUE);

  // Mock expectation for semaphore deletion during cleanup
  vSemaphoreDelete_CMockExpect(13, (SemaphoreHandle_t)0x2000);

  // More semaphore expectations
  xSemaphoreTake_CMockExpectAndReturn(14, (SemaphoreHandle_t)0x2000, portMAX_DELAY, pdTRUE);
  xSemaphoreGive_CMockExpectAndReturn(15, (SemaphoreHandle_t)0x2000, pdTRUE);
  xSemaphoreTake_CMockExpectAndReturn(16, (SemaphoreHandle_t)0x2000, portMAX_DELAY, pdTRUE);
  xSemaphoreGive_CMockExpectAndReturn(17, (SemaphoreHandle_t)0x2000, pdTRUE);

  // Mock expectation for heap free during cleanup
  heap_caps_free_CMockExpect(18, mock_buffer_10);

  // Initialize buffer
  TEST_ASSERT_TRUE(circular_buffer_init(&test_buf, sizeof(test_item_t), 10));

  test_item_t item = {456, "clear_test"};

  // Add item and clear
  TEST_ASSERT_TRUE(circular_buffer_push(&test_buf, &item));
  TEST_ASSERT_EQUAL(1, circular_buffer_count(&test_buf));
  TEST_ASSERT_FALSE(circular_buffer_is_empty(&test_buf));

  circular_buffer_clear(&test_buf);
  TEST_ASSERT_EQUAL(0, circular_buffer_count(&test_buf));
  TEST_ASSERT_TRUE(circular_buffer_is_empty(&test_buf));

  // Clean up
  circular_buffer_free(&test_buf);
}

// Test get_at_index with various indices
void test_circular_buffer_get_at_index(void)
{
  circular_buffer_t test_buf;

  // Initialize CMock mocks
  Mockmock_semphr_Init();
  Mockmock_esp_heap_caps_Init();

  // Set up mock expectations for init
  heap_caps_malloc_CMockExpectAndReturn(1, 10 * sizeof(test_item_t), MALLOC_CAP_SPIRAM, mock_buffer_10);
  xSemaphoreCreateMutex_CMockExpectAndReturn(2, (SemaphoreHandle_t)0x2000);

  // Mock expectations for semaphore operations during operations
  // 5 push operations + 7 get operations = 24 semaphore operations
  for (int i = 3; i <= 50; i += 2)
  {
    xSemaphoreTake_CMockExpectAndReturn(i, (SemaphoreHandle_t)0x2000, portMAX_DELAY, pdTRUE);
    xSemaphoreGive_CMockExpectAndReturn(i + 1, (SemaphoreHandle_t)0x2000, pdTRUE);
  }

  // Mock expectation for semaphore deletion during cleanup
  vSemaphoreDelete_CMockExpect(51, (SemaphoreHandle_t)0x2000);

  // Mock expectation for heap free during cleanup
  heap_caps_free_CMockExpect(52, mock_buffer_10);

  // Initialize buffer
  TEST_ASSERT_TRUE(circular_buffer_init(&test_buf, sizeof(test_item_t), 10));

  test_item_t item;
  test_item_t result;

  // Add 5 items
  for (int i = 0; i < 5; i++)
  {
    item.value = i * 10;
    sprintf(item.name, "item_%d", i);
    TEST_ASSERT_TRUE(circular_buffer_push(&test_buf, &item));
  }

  // Test valid indices
  char expected_name[32];
  for (int i = 0; i < 5; i++)
  {
    TEST_ASSERT_TRUE(circular_buffer_get_at_index(&test_buf, i, &result));
    TEST_ASSERT_EQUAL(i * 10, result.value);
    sprintf(expected_name, "item_%d", i);
    TEST_ASSERT_EQUAL(0, strcmp(result.name, expected_name));
  }

  // Test invalid indices
  TEST_ASSERT_FALSE(circular_buffer_get_at_index(&test_buf, 5, &result));  // Out of bounds
  TEST_ASSERT_FALSE(circular_buffer_get_at_index(&test_buf, 10, &result)); // Way out of bounds

  // Clean up
  circular_buffer_free(&test_buf);
}

// Test NULL parameter handling
void test_circular_buffer_null_parameters(void)
{
  circular_buffer_t *null_cb = NULL;
  test_item_t item = {123, "test"};
  test_item_t result;

  // All functions should handle NULL circular_buffer_t gracefully
  TEST_ASSERT_FALSE(circular_buffer_push(null_cb, &item));
  TEST_ASSERT_FALSE(circular_buffer_get_latest(null_cb, &result));
  TEST_ASSERT_FALSE(circular_buffer_get_at_index(null_cb, 0, &result));
  TEST_ASSERT_EQUAL(0, circular_buffer_count(null_cb));
  TEST_ASSERT_FALSE(circular_buffer_is_full(null_cb));
  TEST_ASSERT_TRUE(circular_buffer_is_empty(null_cb)); // Should return true for NULL
  circular_buffer_clear(null_cb);                      // Should not crash
  circular_buffer_free(null_cb);                       // Should not crash

  // Test with initialized buffer but NULL data pointers
  circular_buffer_t test_buf;

  // Initialize CMock mocks
  Mockmock_semphr_Init();
  Mockmock_esp_heap_caps_Init();

  // Set up mock expectations for init
  heap_caps_malloc_CMockExpectAndReturn(1, 10 * sizeof(test_item_t), MALLOC_CAP_SPIRAM, mock_buffer_10);
  xSemaphoreCreateMutex_CMockExpectAndReturn(2, (SemaphoreHandle_t)0x2000);

  // Initialize buffer
  TEST_ASSERT_TRUE(circular_buffer_init(&test_buf, sizeof(test_item_t), 10));

  // Mock expectations for semaphore operations during operations
  xSemaphoreTake_CMockExpectAndReturn(3, (SemaphoreHandle_t)0x2000, portMAX_DELAY, pdTRUE);
  xSemaphoreGive_CMockExpectAndReturn(4, (SemaphoreHandle_t)0x2000, pdTRUE);
  xSemaphoreTake_CMockExpectAndReturn(5, (SemaphoreHandle_t)0x2000, portMAX_DELAY, pdTRUE);
  xSemaphoreGive_CMockExpectAndReturn(6, (SemaphoreHandle_t)0x2000, pdTRUE);
  xSemaphoreTake_CMockExpectAndReturn(7, (SemaphoreHandle_t)0x2000, portMAX_DELAY, pdTRUE);
  xSemaphoreGive_CMockExpectAndReturn(8, (SemaphoreHandle_t)0x2000, pdTRUE);

  TEST_ASSERT_FALSE(circular_buffer_push(&test_buf, NULL));
  TEST_ASSERT_FALSE(circular_buffer_get_latest(&test_buf, NULL));
  TEST_ASSERT_FALSE(circular_buffer_get_at_index(&test_buf, 0, NULL));

  // Mock expectation for semaphore deletion during cleanup
  vSemaphoreDelete_CMockExpect(9, (SemaphoreHandle_t)0x2000);

  // Mock expectation for heap free during cleanup
  heap_caps_free_CMockExpect(10, mock_buffer_10);

  // Clean up
  circular_buffer_free(&test_buf);
}

// Test group runner
void test_circular_buffer_real(void)
{
  printf("Running real circular buffer tests...\n");
  RUN_TEST(test_circular_buffer_init);
  RUN_TEST(test_circular_buffer_init_failures);
  RUN_TEST(test_circular_buffer_push_pop);
  RUN_TEST(test_circular_buffer_overflow);
  RUN_TEST(test_circular_buffer_empty);
  RUN_TEST(test_circular_buffer_clear);
  RUN_TEST(test_circular_buffer_get_at_index);
  RUN_TEST(test_circular_buffer_null_parameters);
  printf("Real circular buffer tests completed\n");
}
