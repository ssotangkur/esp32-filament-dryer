#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "unity.h"

#include "Mockmock_esp_heap_caps.h"
#include "Mockmock_semphr.h"

#include "../../include/circular_buffer.h"

typedef struct
{
  int value;
  char name[32];
} test_item_t;

static circular_buffer_t test_buffer;

SemaphoreHandle_t SEMAPHORE_1 = 0x2000;

void *mock_circular_buffer_init_success(circular_buffer_t *cb, size_t element_size, size_t buffer_size)
{
  size_t mem_size = element_size * buffer_size;
  void *mock_buffer = malloc(mem_size);
  heap_caps_malloc_ExpectAndReturn(mem_size, MALLOC_CAP_SPIRAM, mock_buffer);
  xSemaphoreCreateMutex_ExpectAndReturn(SEMAPHORE_1);
  cb->mutex = SEMAPHORE_1;
  TEST_ASSERT_TRUE(circular_buffer_init(cb, element_size, buffer_size));
  return mock_buffer;
}

void mock_circular_buffer_free(circular_buffer_t *cb, void *buffer)
{
  if (cb->subscribers != NULL)
  {
    heap_caps_free_Expect(cb->subscribers);
  }
  if (cb->mutex != NULL)
  {
    vSemaphoreDelete_Expect(cb->mutex);
  }
  if (buffer != NULL)
  {
    heap_caps_free_Expect(buffer);
  }
  circular_buffer_free(cb);
}

void mock_circular_buffer_push(circular_buffer_t *cb, const void *item)
{
  xSemaphoreTake_ExpectAndReturn(cb->mutex, portMAX_DELAY, pdTRUE);
  xSemaphoreGive_ExpectAndReturn(cb->mutex, pdTRUE);
  circular_buffer_push(cb, item);
}

size_t mock_circular_buffer_count(circular_buffer_t *cb)
{
  xSemaphoreTake_ExpectAndReturn(cb->mutex, portMAX_DELAY, pdTRUE);
  xSemaphoreGive_ExpectAndReturn(cb->mutex, pdTRUE);
  return circular_buffer_count(cb);
}

bool mock_circular_buffer_is_empty(circular_buffer_t *cb)
{
  xSemaphoreTake_ExpectAndReturn(cb->mutex, portMAX_DELAY, pdTRUE);
  xSemaphoreGive_ExpectAndReturn(cb->mutex, pdTRUE);
  return circular_buffer_is_empty(cb);
}

bool mock_circular_buffer_is_full(circular_buffer_t *cb)
{
  xSemaphoreTake_ExpectAndReturn(cb->mutex, portMAX_DELAY, pdTRUE);
  xSemaphoreGive_ExpectAndReturn(cb->mutex, pdTRUE);
  return circular_buffer_is_full(cb);
}

bool mock_circular_buffer_get_latest(circular_buffer_t *cb, void *item)
{
  xSemaphoreTake_ExpectAndReturn(cb->mutex, portMAX_DELAY, pdTRUE);
  xSemaphoreGive_ExpectAndReturn(cb->mutex, pdTRUE);
  return circular_buffer_get_latest(cb, item);
}

bool mock_circular_buffer_get_at_index(circular_buffer_t *cb, size_t index, void *item)
{
  xSemaphoreTake_ExpectAndReturn(cb->mutex, portMAX_DELAY, pdTRUE);
  xSemaphoreGive_ExpectAndReturn(cb->mutex, pdTRUE);
  return circular_buffer_get_at_index(cb, index, item);
}

void mock_circular_buffer_clear(circular_buffer_t *cb)
{
  xSemaphoreTake_ExpectAndReturn(cb->mutex, portMAX_DELAY, pdTRUE);
  xSemaphoreGive_ExpectAndReturn(cb->mutex, pdTRUE);
  circular_buffer_clear(cb);
}

void test_circular_buffer_init(void)
{
  circular_buffer_t test_buf;
  void *buffer = mock_circular_buffer_init_success(&test_buf, sizeof(test_item_t), 5);
  TEST_ASSERT_EQUAL(5, test_buf.buffer_size);
  TEST_ASSERT_EQUAL(0, mock_circular_buffer_count(&test_buf));
  TEST_ASSERT_TRUE(mock_circular_buffer_is_empty(&test_buf));
  TEST_ASSERT_FALSE(mock_circular_buffer_is_full(&test_buf));
  mock_circular_buffer_free(&test_buf, buffer);
}

void test_circular_buffer_init_failures(void)
{
  circular_buffer_t invalid_buffer;
  TEST_ASSERT_FALSE(circular_buffer_init(NULL, sizeof(test_item_t), 5));
  TEST_ASSERT_FALSE(circular_buffer_init(&invalid_buffer, 0, 5));
  TEST_ASSERT_FALSE(circular_buffer_init(&invalid_buffer, sizeof(test_item_t), 0));
}

void test_circular_buffer_push_pop(void)
{
  circular_buffer_t test_buf;
  void *buffer = mock_circular_buffer_init_success(&test_buf, sizeof(test_item_t), 10);

  test_item_t item1 = {42, "test_item_1"};
  test_item_t item2 = {84, "test_item_2"};
  test_item_t result;

  mock_circular_buffer_push(&test_buf, &item1);
  TEST_ASSERT_EQUAL(1, mock_circular_buffer_count(&test_buf));

  TEST_ASSERT_FALSE(mock_circular_buffer_is_empty(&test_buf));

  mock_circular_buffer_push(&test_buf, &item2);
  TEST_ASSERT_EQUAL(2, mock_circular_buffer_count(&test_buf));

  mock_circular_buffer_get_latest(&test_buf, &result);
  TEST_ASSERT_EQUAL(84, result.value);
  TEST_ASSERT_EQUAL(0, strcmp("test_item_2", result.name));

  mock_circular_buffer_get_at_index(&test_buf, 0, &result);
  TEST_ASSERT_EQUAL(42, result.value);

  mock_circular_buffer_free(&test_buf, buffer);
}

void test_circular_buffer_overflow(void)
{
  circular_buffer_t test_buf;
  void *buffer = mock_circular_buffer_init_success(&test_buf, sizeof(test_item_t), 10);

  test_item_t item;

  for (int i = 0; i < test_buf.buffer_size; i++)
  {
    item.value = i;
    sprintf(item.name, "item_%d", i);
    mock_circular_buffer_push(&test_buf, &item);
  }

  TEST_ASSERT_EQUAL(test_buf.buffer_size, mock_circular_buffer_count(&test_buf));

  item.value = 999;
  sprintf(item.name, "overflow_item");
  mock_circular_buffer_push(&test_buf, &item);

  TEST_ASSERT_EQUAL(test_buf.buffer_size, mock_circular_buffer_count(&test_buf));

  mock_circular_buffer_free(&test_buf, buffer);
}

void test_circular_buffer_empty(void)
{
  circular_buffer_t test_buf;
  void *buffer = mock_circular_buffer_init_success(&test_buf, sizeof(test_item_t), 10);

  test_item_t result;

  mock_circular_buffer_get_latest(&test_buf, &result);
  TEST_ASSERT_FALSE(mock_circular_buffer_get_latest(&test_buf, &result));
  TEST_ASSERT_EQUAL(0, mock_circular_buffer_count(&test_buf));
  TEST_ASSERT_TRUE(mock_circular_buffer_is_empty(&test_buf));

  mock_circular_buffer_free(&test_buf, buffer);
}

void test_circular_buffer_clear(void)
{
  circular_buffer_t test_buf;
  void *buffer = mock_circular_buffer_init_success(&test_buf, sizeof(test_item_t), 10);

  test_item_t item = {456, "clear_test"};

  mock_circular_buffer_push(&test_buf, &item);
  TEST_ASSERT_EQUAL(1, mock_circular_buffer_count(&test_buf));
  TEST_ASSERT_FALSE(mock_circular_buffer_is_empty(&test_buf));

  mock_circular_buffer_clear(&test_buf);

  TEST_ASSERT_EQUAL(0, mock_circular_buffer_count(&test_buf));
  TEST_ASSERT_TRUE(mock_circular_buffer_is_empty(&test_buf));

  mock_circular_buffer_free(&test_buf, buffer);
}

void test_circular_buffer_get_at_index(void)
{
  circular_buffer_t test_buf;
  void *buffer = mock_circular_buffer_init_success(&test_buf, sizeof(test_item_t), 10);

  test_item_t item;
  test_item_t result;

  for (int i = 0; i < 5; i++)
  {
    item.value = i * 10;
    sprintf(item.name, "item_%d", i);
    mock_circular_buffer_push(&test_buf, &item);
  }

  char expected_name[32];
  for (int i = 0; i < 5; i++)
  {
    mock_circular_buffer_get_at_index(&test_buf, i, &result);
    TEST_ASSERT_EQUAL(i * 10, result.value);
    sprintf(expected_name, "item_%d", i);
    TEST_ASSERT_EQUAL(0, strcmp(result.name, expected_name));
  }

  mock_circular_buffer_get_at_index(&test_buf, 5, &result);
  TEST_ASSERT_FALSE(mock_circular_buffer_get_at_index(&test_buf, 5, &result));

  mock_circular_buffer_get_at_index(&test_buf, 10, &result);
  TEST_ASSERT_FALSE(mock_circular_buffer_get_at_index(&test_buf, 10, &result));

  mock_circular_buffer_free(&test_buf, buffer);
}

void test_circular_buffer_null_parameters(void)
{
  circular_buffer_t *null_cb = NULL;
  test_item_t item = {123, "test"};
  test_item_t result;

  TEST_ASSERT_FALSE(circular_buffer_push(null_cb, &item));
  TEST_ASSERT_FALSE(circular_buffer_get_latest(null_cb, &result));
  TEST_ASSERT_FALSE(circular_buffer_get_at_index(null_cb, 0, &result));
  TEST_ASSERT_EQUAL(0, circular_buffer_count(null_cb));
  TEST_ASSERT_FALSE(circular_buffer_is_full(null_cb));
  TEST_ASSERT_TRUE(circular_buffer_is_empty(null_cb));
  circular_buffer_clear(null_cb);
  circular_buffer_free(null_cb);

  circular_buffer_t test_buf;
  void *buffer = mock_circular_buffer_init_success(&test_buf, sizeof(test_item_t), 10);

  TEST_ASSERT_FALSE(circular_buffer_push(&test_buf, NULL));
  TEST_ASSERT_FALSE(circular_buffer_get_latest(&test_buf, NULL));
  TEST_ASSERT_FALSE(circular_buffer_get_at_index(&test_buf, 0, NULL));

  mock_circular_buffer_free(&test_buf, buffer);
}

// Subscriber test helpers
static bool g_subscribe_callback_called = false;
static size_t g_subscribe_callback_count = 0;
static size_t g_subscribe_data_count = 0;
static void *g_subscribe_data = NULL;

static void test_subscribe_callback(const void *data, size_t count, void *user_data)
{
  g_subscribe_callback_called = true;
  g_subscribe_callback_count++;
  g_subscribe_data_count = count;
  g_subscribe_data = (void *)data;
  (void)user_data;
}

static void reset_subscriber_globals(void)
{
  g_subscribe_callback_called = false;
  g_subscribe_callback_count = 0;
  g_subscribe_data_count = 0;
  g_subscribe_data = NULL;
}

bool mock_circular_buffer_subscribe(circular_buffer_t *cb, circular_buffer_subscriber_cb_t callback, void *user_data)
{
  if (cb == NULL || callback == NULL)
  {
    return circular_buffer_subscribe(cb, callback, user_data);
  }

  xSemaphoreTake_ExpectAndReturn(cb->mutex, portMAX_DELAY, pdTRUE);

  // Allocate real memory for new_subscribers - match actual size calculation
  size_t new_size = (cb->subscriber_count + 1) * sizeof(circular_buffer_subscriber_t);
  circular_buffer_subscriber_t *new_subscribers = malloc(new_size);
  heap_caps_malloc_ExpectAndReturn(new_size, MALLOC_CAP_SPIRAM, new_subscribers);

  if (cb->subscribers != NULL)
  {
    heap_caps_free_Expect(cb->subscribers);
  }

  // If there's historical data, allocate and free temp buffer
  if (cb->count > 0)
  {
    size_t hist_size = cb->count * cb->element_size;
    void *hist_data = malloc(hist_size);
    heap_caps_malloc_ExpectAndReturn(hist_size, MALLOC_CAP_SPIRAM, hist_data);
    heap_caps_free_Expect(hist_data);
  }

  xSemaphoreGive_ExpectAndReturn(cb->mutex, pdTRUE);

  return circular_buffer_subscribe(cb, callback, user_data);
}

bool mock_circular_buffer_unsubscribe(circular_buffer_t *cb, void *user_data)
{
  if (cb == NULL)
  {
    return circular_buffer_unsubscribe(cb, user_data);
  }

  xSemaphoreTake_ExpectAndReturn(cb->mutex, portMAX_DELAY, pdTRUE);

  if (cb->subscriber_count > 0)
  {
    if (cb->subscriber_count == 1)
    {
      heap_caps_free_Expect(cb->subscribers);
    }
    else
    {
      size_t new_size = cb->subscriber_count * sizeof(circular_buffer_subscriber_t);
      circular_buffer_subscriber_t *new_subscribers = malloc(new_size);
      heap_caps_malloc_ExpectAndReturn(new_size, MALLOC_CAP_SPIRAM, new_subscribers);
      heap_caps_free_Expect(cb->subscribers);
    }
  }

  xSemaphoreGive_ExpectAndReturn(cb->mutex, pdTRUE);

  return circular_buffer_unsubscribe(cb, user_data);
}

// Wrapper for unsubscribe that expects failure (subscriber not found)
bool mock_circular_buffer_unsubscribe_not_found(circular_buffer_t *cb, void *user_data)
{
  if (cb == NULL)
  {
    return circular_buffer_unsubscribe(cb, user_data);
  }

  xSemaphoreTake_ExpectAndReturn(cb->mutex, portMAX_DELAY, pdTRUE);
  xSemaphoreGive_ExpectAndReturn(cb->mutex, pdTRUE);

  return circular_buffer_unsubscribe(cb, user_data);
}

bool mock_circular_buffer_iterate(circular_buffer_t *cb, bool (*callback)(size_t index, const void *data, void *user_data), void *user_data)
{
  if (cb == NULL || callback == NULL)
  {
    return circular_buffer_iterate(cb, callback, user_data);
  }

  xSemaphoreTake_ExpectAndReturn(cb->mutex, portMAX_DELAY, pdTRUE);
  xSemaphoreGive_ExpectAndReturn(cb->mutex, pdTRUE);

  return circular_buffer_iterate(cb, callback, user_data);
}

bool test_iterate_callback(size_t index, const void *data, void *user_data)
{
  g_subscribe_callback_called = true;
  g_subscribe_callback_count++;
  (void)index;
  (void)data;
  (void)user_data;
  return true;
}

void test_circular_buffer_subscribe_empty_buffer(void)
{
  circular_buffer_t test_buf;
  void *buffer = mock_circular_buffer_init_success(&test_buf, sizeof(test_item_t), 10);
  reset_subscriber_globals();

  // For empty buffer, subscribe should succeed but no historical data callback
  TEST_ASSERT_TRUE(mock_circular_buffer_subscribe(&test_buf, test_subscribe_callback, (void *)0x1234));

  mock_circular_buffer_free(&test_buf, buffer);
}

void test_circular_buffer_subscribe_with_data(void)
{
  circular_buffer_t test_buf;
  void *buffer = mock_circular_buffer_init_success(&test_buf, sizeof(test_item_t), 10);
  reset_subscriber_globals();

  test_item_t item1 = {100, "first"};
  test_item_t item2 = {200, "second"};

  mock_circular_buffer_push(&test_buf, &item1);
  mock_circular_buffer_push(&test_buf, &item2);
  TEST_ASSERT_EQUAL(2, mock_circular_buffer_count(&test_buf));

  // Subscribe - should receive all historical data
  TEST_ASSERT_TRUE(mock_circular_buffer_subscribe(&test_buf, test_subscribe_callback, (void *)0x1234));

  mock_circular_buffer_free(&test_buf, buffer);
}

void test_circular_buffer_subscribe_null_params(void)
{
  circular_buffer_t test_buf;
  void *buffer = mock_circular_buffer_init_success(&test_buf, sizeof(test_item_t), 10);

  TEST_ASSERT_FALSE(circular_buffer_subscribe(NULL, test_subscribe_callback, NULL));
  TEST_ASSERT_FALSE(circular_buffer_subscribe(&test_buf, NULL, NULL));

  mock_circular_buffer_free(&test_buf, buffer);
}

void test_circular_buffer_unsubscribe(void)
{
  circular_buffer_t test_buf;
  void *buffer = mock_circular_buffer_init_success(&test_buf, sizeof(test_item_t), 10);
  reset_subscriber_globals();

  void *user_data = (void *)0x1234;

  TEST_ASSERT_TRUE(mock_circular_buffer_subscribe(&test_buf, test_subscribe_callback, user_data));

  TEST_ASSERT_TRUE(mock_circular_buffer_unsubscribe(&test_buf, user_data));

  mock_circular_buffer_free(&test_buf, buffer);
}

void test_circular_buffer_unsubscribe_not_found(void)
{
  circular_buffer_t test_buf;
  void *buffer = mock_circular_buffer_init_success(&test_buf, sizeof(test_item_t), 10);

  void *user_data1 = (void *)0x1234;
  void *user_data2 = (void *)0x5678;

  TEST_ASSERT_TRUE(mock_circular_buffer_subscribe(&test_buf, test_subscribe_callback, user_data1));

  // Try to unsubscribe with different user_data - should fail (subscriber not found)
  TEST_ASSERT_FALSE(mock_circular_buffer_unsubscribe_not_found(&test_buf, user_data2));

  mock_circular_buffer_free(&test_buf, buffer);
}

void test_circular_buffer_multiple_subscribers(void)
{
  circular_buffer_t test_buf;
  void *buffer = mock_circular_buffer_init_success(&test_buf, sizeof(test_item_t), 10);
  reset_subscriber_globals();

  void *user_data1 = (void *)0x1234;
  void *user_data2 = (void *)0x5678;

  TEST_ASSERT_TRUE(mock_circular_buffer_subscribe(&test_buf, test_subscribe_callback, user_data1));
  TEST_ASSERT_TRUE(mock_circular_buffer_subscribe(&test_buf, test_subscribe_callback, user_data2));

  mock_circular_buffer_free(&test_buf, buffer);
}

void test_circular_buffer_push_notifies_subscribers(void)
{
  circular_buffer_t test_buf;
  void *buffer = mock_circular_buffer_init_success(&test_buf, sizeof(test_item_t), 10);
  reset_subscriber_globals();

  // Subscribe first
  TEST_ASSERT_TRUE(mock_circular_buffer_subscribe(&test_buf, test_subscribe_callback, (void *)0x1234));

  // Now push - subscriber should be notified (semaphore ops for push)
  xSemaphoreTake_ExpectAndReturn(test_buf.mutex, portMAX_DELAY, pdTRUE);
  xSemaphoreGive_ExpectAndReturn(test_buf.mutex, pdTRUE);

  test_item_t item = {42, "test"};
  circular_buffer_push(&test_buf, &item);

  mock_circular_buffer_free(&test_buf, buffer);
}

void test_circular_buffer_iterate(void)
{
  circular_buffer_t test_buf;
  void *buffer = mock_circular_buffer_init_success(&test_buf, sizeof(test_item_t), 10);
  reset_subscriber_globals();

  test_item_t item;
  for (int i = 0; i < 3; i++)
  {
    item.value = i * 10;
    sprintf(item.name, "item_%d", i);
    mock_circular_buffer_push(&test_buf, &item);
  }

  TEST_ASSERT_TRUE(mock_circular_buffer_iterate(&test_buf, test_iterate_callback, NULL));

  mock_circular_buffer_free(&test_buf, buffer);
}

void test_circular_buffer_iterate_empty(void)
{
  circular_buffer_t test_buf;
  void *buffer = mock_circular_buffer_init_success(&test_buf, sizeof(test_item_t), 10);
  reset_subscriber_globals();

  TEST_ASSERT_TRUE(mock_circular_buffer_iterate(&test_buf, test_iterate_callback, NULL));

  mock_circular_buffer_free(&test_buf, buffer);
}

void test_circular_buffer_iterate_null_params(void)
{
  circular_buffer_t test_buf;
  void *buffer = mock_circular_buffer_init_success(&test_buf, sizeof(test_item_t), 10);

  TEST_ASSERT_FALSE(circular_buffer_iterate(NULL, test_iterate_callback, NULL));
  TEST_ASSERT_FALSE(circular_buffer_iterate(&test_buf, NULL, NULL));

  mock_circular_buffer_free(&test_buf, buffer);
}

void test_circular_buffer_subscriber_system_real(void)
{
  printf("Running circular buffer subscriber system tests...\n");

  RUN_TEST(test_circular_buffer_subscribe_empty_buffer);
  RUN_TEST(test_circular_buffer_subscribe_with_data);
  RUN_TEST(test_circular_buffer_subscribe_null_params);
  RUN_TEST(test_circular_buffer_unsubscribe);
  RUN_TEST(test_circular_buffer_unsubscribe_not_found);
  RUN_TEST(test_circular_buffer_multiple_subscribers);
  RUN_TEST(test_circular_buffer_push_notifies_subscribers);
  RUN_TEST(test_circular_buffer_iterate);
  RUN_TEST(test_circular_buffer_iterate_empty);
  RUN_TEST(test_circular_buffer_iterate_null_params);

  printf("Circular buffer subscriber system tests completed\n");
}

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
