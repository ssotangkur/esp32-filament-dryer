#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "circular_buffer_simple.c"
#include "test_framework.h"

// Test data structure
typedef struct
{
  int value;
  char name[32];
} test_item_t;

// Test buffer
static circular_buffer_t test_buffer;

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
  init_test_buffer();
  TEST_ASSERT_TRUE(circular_buffer_init(&test_buffer, sizeof(test_item_t), 5));
  TEST_ASSERT_EQUAL(0, circular_buffer_count(&test_buffer));
  TEST_ASSERT_EQUAL(5, test_buffer.buffer_size);
  TEST_ASSERT_TRUE(circular_buffer_is_empty(&test_buffer));
  TEST_ASSERT_FALSE(circular_buffer_is_full(&test_buffer));
  cleanup_test_buffer();
}

// Test basic push and pop operations
void test_circular_buffer_push_pop(void)
{
  init_test_buffer();
  test_item_t item1 = {42, "test_item_1"};
  test_item_t item2 = {84, "test_item_2"};
  test_item_t result;

  // Push items
  TEST_ASSERT_TRUE(circular_buffer_push(&test_buffer, &item1));
  TEST_ASSERT_EQUAL(1, circular_buffer_count(&test_buffer));
  TEST_ASSERT_FALSE(circular_buffer_is_empty(&test_buffer));

  TEST_ASSERT_TRUE(circular_buffer_push(&test_buffer, &item2));
  TEST_ASSERT_EQUAL(2, circular_buffer_count(&test_buffer));

  // Get latest (should be item2)
  TEST_ASSERT_TRUE(circular_buffer_get_latest(&test_buffer, &result));
  TEST_ASSERT_EQUAL(84, result.value);
  TEST_ASSERT_EQUAL(0, strcmp("test_item_2", result.name));
  TEST_ASSERT_EQUAL(2, circular_buffer_count(&test_buffer)); // get_latest doesn't remove

  // Get at index 0 (should be item1)
  TEST_ASSERT_TRUE(circular_buffer_get_at_index(&test_buffer, 0, &result));
  TEST_ASSERT_EQUAL(42, result.value);
  TEST_ASSERT_EQUAL(0, strcmp("test_item_1", result.name));

  cleanup_test_buffer();
}

// Test buffer overflow behavior
void test_circular_buffer_overflow(void)
{
  init_test_buffer();
  test_item_t item;
  test_item_t result;

  // Fill buffer to capacity
  for (int i = 0; i < test_buffer.buffer_size; i++)
  {
    item.value = i;
    sprintf(item.name, "item_%d", i);
    TEST_ASSERT_TRUE(circular_buffer_push(&test_buffer, &item));
  }

  TEST_ASSERT_EQUAL(test_buffer.buffer_size, circular_buffer_count(&test_buffer));
  TEST_ASSERT_TRUE(circular_buffer_is_full(&test_buffer));

  // Try to push one more (should overwrite oldest)
  item.value = 999;
  sprintf(item.name, "overflow_item");
  TEST_ASSERT_TRUE(circular_buffer_push(&test_buffer, &item));
  TEST_ASSERT_EQUAL(test_buffer.buffer_size, circular_buffer_count(&test_buffer));
  TEST_ASSERT_TRUE(circular_buffer_is_full(&test_buffer));

  // Check that oldest item was overwritten
  TEST_ASSERT_TRUE(circular_buffer_get_at_index(&test_buffer, 0, &result));
  TEST_ASSERT_EQUAL(999, result.value); // Should be the overflow item

  cleanup_test_buffer();
}

// Test buffer empty condition
void test_circular_buffer_empty(void)
{
  init_test_buffer();
  test_item_t result;
  TEST_ASSERT_FALSE(circular_buffer_get_latest(&test_buffer, &result));
  TEST_ASSERT_EQUAL(0, circular_buffer_count(&test_buffer));
  TEST_ASSERT_TRUE(circular_buffer_is_empty(&test_buffer));
  cleanup_test_buffer();
}

// Test buffer clear operation
void test_circular_buffer_clear(void)
{
  init_test_buffer();
  test_item_t item = {456, "clear_test"};

  // Add item and clear
  TEST_ASSERT_TRUE(circular_buffer_push(&test_buffer, &item));
  TEST_ASSERT_EQUAL(1, circular_buffer_count(&test_buffer));
  TEST_ASSERT_FALSE(circular_buffer_is_empty(&test_buffer));

  circular_buffer_clear(&test_buffer);
  TEST_ASSERT_EQUAL(0, circular_buffer_count(&test_buffer));
  TEST_ASSERT_TRUE(circular_buffer_is_empty(&test_buffer));
  cleanup_test_buffer();
}

// Test group runner
void test_circular_buffer(void)
{
  printf("Running circular buffer tests...\n");
  test_circular_buffer_init();
  test_circular_buffer_push_pop();
  test_circular_buffer_overflow();
  test_circular_buffer_empty();
  test_circular_buffer_clear();
  printf("Circular buffer tests completed: %d passed, %d failed\n", tests_run, failures);
}
