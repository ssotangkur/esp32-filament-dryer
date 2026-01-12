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

// Mock wrapper functions for better test readability and maintainability

/**
 * @brief Setup mocks and call circular_buffer_init for successful initialization
 * @param call_count_ptr Pointer to call count (will be updated)
 * @param cb Pointer to circular buffer to initialize
 * @param element_size Size of each element
 * @param buffer_size Number of elements in buffer
 * @param mock_buffer Pointer to mock memory buffer
 * @return Result of circular_buffer_init call (should be true)
 */
bool mock_circular_buffer_init_success(int *call_count_ptr, circular_buffer_t *cb, size_t element_size, size_t buffer_size, void *mock_buffer)
{
  // Mock memory allocation
  heap_caps_malloc_CMockExpectAndReturn((*call_count_ptr)++, element_size * buffer_size, MALLOC_CAP_SPIRAM, mock_buffer);

  // Mock mutex creation
  xSemaphoreCreateMutex_CMockExpectAndReturn((*call_count_ptr)++, (SemaphoreHandle_t)0x2000);

  return circular_buffer_init(cb, element_size, buffer_size);
}

/**
 * @brief Setup mocks and call circular_buffer_count operation
 * @param call_count_ptr Pointer to call count (will be updated)
 * @param cb Pointer to circular buffer
 * @return Result of circular_buffer_count call
 */
size_t mock_circular_buffer_count(int *call_count_ptr, circular_buffer_t *cb)
{
  xSemaphoreTake_CMockExpectAndReturn((*call_count_ptr)++, (SemaphoreHandle_t)0x2000, portMAX_DELAY, pdTRUE);
  xSemaphoreGive_CMockExpectAndReturn((*call_count_ptr)++, (SemaphoreHandle_t)0x2000, pdTRUE);
  return circular_buffer_count(cb);
}

/**
 * @brief Setup mocks and call circular_buffer_is_empty operation
 * @param call_count_ptr Pointer to call count (will be updated)
 * @param cb Pointer to circular buffer
 * @return Result of circular_buffer_is_empty call
 */
bool mock_circular_buffer_is_empty(int *call_count_ptr, circular_buffer_t *cb)
{
  xSemaphoreTake_CMockExpectAndReturn((*call_count_ptr)++, (SemaphoreHandle_t)0x2000, portMAX_DELAY, pdTRUE);
  xSemaphoreGive_CMockExpectAndReturn((*call_count_ptr)++, (SemaphoreHandle_t)0x2000, pdTRUE);
  return circular_buffer_is_empty(cb);
}

/**
 * @brief Setup mocks and call circular_buffer_is_full operation
 * @param call_count_ptr Pointer to call count (will be updated)
 * @param cb Pointer to circular buffer
 * @return Result of circular_buffer_is_full call
 */
bool mock_circular_buffer_is_full(int *call_count_ptr, circular_buffer_t *cb)
{
  xSemaphoreTake_CMockExpectAndReturn((*call_count_ptr)++, (SemaphoreHandle_t)0x2000, portMAX_DELAY, pdTRUE);
  xSemaphoreGive_CMockExpectAndReturn((*call_count_ptr)++, (SemaphoreHandle_t)0x2000, pdTRUE);
  return circular_buffer_is_full(cb);
}

/**
 * @brief Setup mocks and call circular_buffer_push operation
 * @param call_count_ptr Pointer to call count (will be updated)
 * @param cb Pointer to circular buffer
 * @param data Pointer to data to push
 * @return Result of circular_buffer_push call
 */
bool mock_circular_buffer_push(int *call_count_ptr, circular_buffer_t *cb, const void *data)
{
  xSemaphoreTake_CMockExpectAndReturn((*call_count_ptr)++, (SemaphoreHandle_t)0x2000, portMAX_DELAY, pdTRUE);
  xSemaphoreGive_CMockExpectAndReturn((*call_count_ptr)++, (SemaphoreHandle_t)0x2000, pdTRUE);
  return circular_buffer_push(cb, data);
}

/**
 * @brief Setup mocks and call circular_buffer_get_latest operation
 * @param call_count_ptr Pointer to call count (will be updated)
 * @param cb Pointer to circular buffer
 * @param data Pointer to buffer for retrieved data
 * @return Result of circular_buffer_get_latest call
 */
bool mock_circular_buffer_get_latest(int *call_count_ptr, circular_buffer_t *cb, void *data)
{
  xSemaphoreTake_CMockExpectAndReturn((*call_count_ptr)++, (SemaphoreHandle_t)0x2000, portMAX_DELAY, pdTRUE);
  xSemaphoreGive_CMockExpectAndReturn((*call_count_ptr)++, (SemaphoreHandle_t)0x2000, pdTRUE);
  return circular_buffer_get_latest(cb, data);
}

/**
 * @brief Setup mocks and call circular_buffer_get_at_index operation
 * @param call_count_ptr Pointer to call count (will be updated)
 * @param cb Pointer to circular buffer
 * @param index Index to retrieve
 * @param data Pointer to buffer for retrieved data
 * @return Result of circular_buffer_get_at_index call
 */
bool mock_circular_buffer_get_at_index(int *call_count_ptr, circular_buffer_t *cb, size_t index, void *data)
{
  xSemaphoreTake_CMockExpectAndReturn((*call_count_ptr)++, (SemaphoreHandle_t)0x2000, portMAX_DELAY, pdTRUE);
  xSemaphoreGive_CMockExpectAndReturn((*call_count_ptr)++, (SemaphoreHandle_t)0x2000, pdTRUE);
  return circular_buffer_get_at_index(cb, index, data);
}

/**
 * @brief Setup mocks for circular buffer cleanup/free
 * @param call_count_start Starting call count for CMock expectations
 * @param mock_buffer Pointer to mock memory buffer
 * @return Next available call count
 */
int mock_circular_buffer_free(int call_count_start, void *mock_buffer)
{
  vSemaphoreDelete_CMockExpect(call_count_start++, (SemaphoreHandle_t)0x2000);
  heap_caps_free_CMockExpect(call_count_start++, mock_buffer);
  return call_count_start;
}

/**
 * @brief Setup mocks and call circular_buffer_init with malloc failure
 * @param call_count_ptr Pointer to call count (will be updated)
 * @param cb Pointer to circular buffer to initialize
 * @param element_size Size of each element
 * @param buffer_size Number of elements in buffer
 * @return Result of circular_buffer_init call (should be false)
 */
bool mock_circular_buffer_init_malloc_failure(int *call_count_ptr, circular_buffer_t *cb, size_t element_size, size_t buffer_size)
{
  // Mock failed memory allocation
  heap_caps_malloc_CMockExpectAndReturn((*call_count_ptr)++, element_size * buffer_size, MALLOC_CAP_SPIRAM, NULL);
  return circular_buffer_init(cb, element_size, buffer_size);
}

/**
 * @brief Setup mocks and call circular_buffer_init with mutex creation failure
 * @param call_count_ptr Pointer to call count (will be updated)
 * @param cb Pointer to circular buffer to initialize
 * @param element_size Size of each element
 * @param buffer_size Number of elements in buffer
 * @param mock_buffer Pointer to mock memory buffer
 * @return Result of circular_buffer_init call (should be false)
 */
bool mock_circular_buffer_init_mutex_failure(int *call_count_ptr, circular_buffer_t *cb, size_t element_size, size_t buffer_size, void *mock_buffer)
{
  // Mock successful memory allocation
  heap_caps_malloc_CMockExpectAndReturn((*call_count_ptr)++, element_size * buffer_size, MALLOC_CAP_SPIRAM, mock_buffer);

  // Mock failed mutex creation
  xSemaphoreCreateMutex_CMockExpectAndReturn((*call_count_ptr)++, NULL);

  // Mock cleanup of allocated memory
  heap_caps_free_CMockExpect((*call_count_ptr)++, mock_buffer);

  return circular_buffer_init(cb, element_size, buffer_size);
}

// Test circular buffer initialization
void test_circular_buffer_init(void)
{
  circular_buffer_t test_buf;
  static uint8_t mock_buffer[5 * sizeof(test_item_t)]; // Real memory for testing

  // Initialize CMock mocks
  Mockmock_semphr_Init();
  Mockmock_esp_heap_caps_Init();

  // Setup mock expectations and call init using wrapper functions
  int call_count = 1;
  TEST_ASSERT_TRUE(mock_circular_buffer_init_success(&call_count, &test_buf, sizeof(test_item_t), 5, mock_buffer));

  // Mock expectations and calls for count/is_empty/is_full checks (3 operations)
  TEST_ASSERT_EQUAL(0, mock_circular_buffer_count(&call_count, &test_buf));
  TEST_ASSERT_TRUE(mock_circular_buffer_is_empty(&call_count, &test_buf));
  TEST_ASSERT_FALSE(mock_circular_buffer_is_full(&call_count, &test_buf));

  TEST_ASSERT_EQUAL(5, test_buf.buffer_size);

  // Mock expectations for cleanup
  call_count = mock_circular_buffer_free(call_count, mock_buffer);

  // Clean up
  circular_buffer_free(&test_buf);
}

// Test initialization failure cases
void test_circular_buffer_init_failures(void)
{
  circular_buffer_t invalid_buffer;

  // Test malloc failure case
  int call_count = 1;
  TEST_ASSERT_FALSE(mock_circular_buffer_init_malloc_failure(&call_count, &invalid_buffer, sizeof(test_item_t), 5));

  // Test mutex failure case
  static uint8_t mock_buffer[5 * sizeof(test_item_t)];
  call_count = 1;
  TEST_ASSERT_FALSE(mock_circular_buffer_init_mutex_failure(&call_count, &invalid_buffer, sizeof(test_item_t), 5, mock_buffer));

  // NULL buffer (no mock setup needed for parameter validation)
  TEST_ASSERT_FALSE(circular_buffer_init(NULL, sizeof(test_item_t), 5));

  // Zero element size (no mock setup needed for parameter validation)
  TEST_ASSERT_FALSE(circular_buffer_init(&invalid_buffer, 0, 5));

  // Zero buffer size (no mock setup needed for parameter validation)
  TEST_ASSERT_FALSE(circular_buffer_init(&invalid_buffer, sizeof(test_item_t), 0));
}

// Test basic push and pop operations
void test_circular_buffer_push_pop(void)
{
  circular_buffer_t test_buf;

  // Initialize CMock mocks
  Mockmock_semphr_Init();
  Mockmock_esp_heap_caps_Init();

  // Initialize buffer using wrapper function
  int call_count = 1;
  TEST_ASSERT_TRUE(mock_circular_buffer_init_success(&call_count, &test_buf, sizeof(test_item_t), 10, mock_buffer_10));

  test_item_t item1 = {42, "test_item_1"};
  test_item_t item2 = {84, "test_item_2"};
  test_item_t result;

  // Push items using wrapper functions
  TEST_ASSERT_TRUE(mock_circular_buffer_push(&call_count, &test_buf, &item1));
  TEST_ASSERT_EQUAL(1, mock_circular_buffer_count(&call_count, &test_buf));
  TEST_ASSERT_FALSE(mock_circular_buffer_is_empty(&call_count, &test_buf));

  TEST_ASSERT_TRUE(mock_circular_buffer_push(&call_count, &test_buf, &item2));
  TEST_ASSERT_EQUAL(2, mock_circular_buffer_count(&call_count, &test_buf));

  // Get latest (should be item2) using wrapper function
  TEST_ASSERT_TRUE(mock_circular_buffer_get_latest(&call_count, &test_buf, &result));
  TEST_ASSERT_EQUAL(84, result.value);
  TEST_ASSERT_EQUAL(0, strcmp("test_item_2", result.name));
  TEST_ASSERT_EQUAL(2, mock_circular_buffer_count(&call_count, &test_buf)); // get_latest doesn't remove

  // Get at index 0 (should be item1) using wrapper function
  TEST_ASSERT_TRUE(mock_circular_buffer_get_at_index(&call_count, &test_buf, 0, &result));
  TEST_ASSERT_EQUAL(42, result.value);
  TEST_ASSERT_EQUAL(0, strcmp("test_item_1", result.name));

  // Clean up using wrapper function
  call_count = mock_circular_buffer_free(call_count, mock_buffer_10);

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

  // Initialize buffer using wrapper function
  int call_count = 1;
  TEST_ASSERT_TRUE(mock_circular_buffer_init_success(&call_count, &test_buf, sizeof(test_item_t), 10, mock_buffer_10));

  test_item_t result;

  // Test operations using wrapper functions
  TEST_ASSERT_FALSE(mock_circular_buffer_get_latest(&call_count, &test_buf, &result));
  TEST_ASSERT_EQUAL(0, mock_circular_buffer_count(&call_count, &test_buf));
  TEST_ASSERT_TRUE(mock_circular_buffer_is_empty(&call_count, &test_buf));

  // Clean up using wrapper function
  call_count = mock_circular_buffer_free(call_count, mock_buffer_10);

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

  // Initialize buffer using wrapper function
  int call_count = 1;
  TEST_ASSERT_TRUE(mock_circular_buffer_init_success(&call_count, &test_buf, sizeof(test_item_t), 10, mock_buffer_10));

  test_item_t item = {456, "clear_test"};

  // Add item using wrapper function
  TEST_ASSERT_TRUE(mock_circular_buffer_push(&call_count, &test_buf, &item));
  TEST_ASSERT_EQUAL(1, mock_circular_buffer_count(&call_count, &test_buf));
  TEST_ASSERT_FALSE(mock_circular_buffer_is_empty(&call_count, &test_buf));

  // Clear operation (this needs manual mock setup since we don't have a wrapper for clear)
  xSemaphoreTake_CMockExpectAndReturn(call_count++, (SemaphoreHandle_t)0x2000, portMAX_DELAY, pdTRUE);
  xSemaphoreGive_CMockExpectAndReturn(call_count++, (SemaphoreHandle_t)0x2000, pdTRUE);
  circular_buffer_clear(&test_buf);

  TEST_ASSERT_EQUAL(0, mock_circular_buffer_count(&call_count, &test_buf));
  TEST_ASSERT_TRUE(mock_circular_buffer_is_empty(&call_count, &test_buf));

  // Clean up using wrapper function
  call_count = mock_circular_buffer_free(call_count, mock_buffer_10);

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

  // Initialize buffer using wrapper function
  int call_count = 1;
  TEST_ASSERT_TRUE(mock_circular_buffer_init_success(&call_count, &test_buf, sizeof(test_item_t), 10, mock_buffer_10));

  test_item_t item;
  test_item_t result;

  // Add 5 items using wrapper functions
  for (int i = 0; i < 5; i++)
  {
    item.value = i * 10;
    sprintf(item.name, "item_%d", i);
    TEST_ASSERT_TRUE(mock_circular_buffer_push(&call_count, &test_buf, &item));
  }

  // Test valid indices using wrapper functions
  char expected_name[32];
  for (int i = 0; i < 5; i++)
  {
    TEST_ASSERT_TRUE(mock_circular_buffer_get_at_index(&call_count, &test_buf, i, &result));
    TEST_ASSERT_EQUAL(i * 10, result.value);
    sprintf(expected_name, "item_%d", i);
    TEST_ASSERT_EQUAL(0, strcmp(result.name, expected_name));
  }

  // Test invalid indices (these need manual mock setup since they still call semaphore operations)
  xSemaphoreTake_CMockExpectAndReturn(call_count++, (SemaphoreHandle_t)0x2000, portMAX_DELAY, pdTRUE);
  xSemaphoreGive_CMockExpectAndReturn(call_count++, (SemaphoreHandle_t)0x2000, pdTRUE);
  TEST_ASSERT_FALSE(circular_buffer_get_at_index(&test_buf, 5, &result)); // Out of bounds

  xSemaphoreTake_CMockExpectAndReturn(call_count++, (SemaphoreHandle_t)0x2000, portMAX_DELAY, pdTRUE);
  xSemaphoreGive_CMockExpectAndReturn(call_count++, (SemaphoreHandle_t)0x2000, pdTRUE);
  TEST_ASSERT_FALSE(circular_buffer_get_at_index(&test_buf, 10, &result)); // Way out of bounds

  // Clean up using wrapper function
  call_count = mock_circular_buffer_free(call_count, mock_buffer_10);

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

  // Initialize buffer using wrapper function
  int call_count = 1;
  TEST_ASSERT_TRUE(mock_circular_buffer_init_success(&call_count, &test_buf, sizeof(test_item_t), 10, mock_buffer_10));

  // Test NULL data pointers (these fail parameter validation so don't need mocks)
  TEST_ASSERT_FALSE(circular_buffer_push(&test_buf, NULL));
  TEST_ASSERT_FALSE(circular_buffer_get_latest(&test_buf, NULL));
  TEST_ASSERT_FALSE(circular_buffer_get_at_index(&test_buf, 0, NULL));

  // Clean up using wrapper function
  call_count = mock_circular_buffer_free(call_count, mock_buffer_10);

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
