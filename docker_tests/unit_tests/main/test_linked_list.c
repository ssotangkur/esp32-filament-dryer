#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "unity.h"

#include "Mockmock_semphr.h"
#include "Mockmock_esp_heap_caps.h"

#include "linked_list.h"

typedef struct {
    int value;
    char name[32];
} test_item_t;

static linked_list_t test_list;
static SemaphoreHandle_t TEST_MUTEX = (SemaphoreHandle_t)0x3000;
static void *TEST_NODE_PTR = (void *)0x2000;

// Mock helper for linked_list_create
void mock_linked_list_create(linked_list_t *list, size_t max_size)
{
    xSemaphoreCreateMutex_ExpectAndReturn(TEST_MUTEX);
    linked_list_create(list, max_size);
}

// Mock helper for linked_list_pop_head
void *mock_linked_list_pop_head(linked_list_t *list, size_t *size)
{
    if (list == NULL || size == NULL) {
        return linked_list_pop_head(list, size);
    }

    xSemaphoreTake_ExpectAndReturn(list->mutex, portMAX_DELAY, pdTRUE);

    if (list->count == 0) {
        xSemaphoreGive_ExpectAndReturn(list->mutex, pdTRUE);
        return linked_list_pop_head(list, size);
    }

    heap_caps_free_ExpectAnyArgs();
    xSemaphoreGive_ExpectAndReturn(list->mutex, pdTRUE);
    return linked_list_pop_head(list, size);
}

// Mock helper for linked_list_pop_tail
void *mock_linked_list_pop_tail(linked_list_t *list, size_t *size)
{
    if (list == NULL || size == NULL) {
        return linked_list_pop_tail(list, size);
    }

    xSemaphoreTake_ExpectAndReturn(list->mutex, portMAX_DELAY, pdTRUE);

    if (list->count == 0) {
        xSemaphoreGive_ExpectAndReturn(list->mutex, pdTRUE);
        return linked_list_pop_tail(list, size);
    }

    heap_caps_free_ExpectAnyArgs();
    xSemaphoreGive_ExpectAndReturn(list->mutex, pdTRUE);
    return linked_list_pop_tail(list, size);
}

// Mock helper for linked_list_enqueue
bool mock_linked_list_enqueue(linked_list_t *list, const void *data, size_t size)
{
    if (list == NULL || data == NULL || size == 0) {
        return linked_list_enqueue(list, data, size);
    }

    // Set up expectations in the order they'll be called
    xSemaphoreTake_ExpectAndReturn(list->mutex, portMAX_DELAY, pdTRUE);

    // Allocate real memory for the node so the real function can access it
    linked_list_node_t *real_node = malloc(sizeof(linked_list_node_t));
    heap_caps_malloc_ExpectAndReturn(sizeof(linked_list_node_t), MALLOC_CAP_SPIRAM, real_node);

    // Finally, xSemaphoreGive is called
    xSemaphoreGive_ExpectAndReturn(list->mutex, pdTRUE);

    return linked_list_enqueue(list, data, size);
}

// Mock helper for linked_list_peek_head
void *mock_linked_list_peek_head(linked_list_t *list, size_t *size)
{
    if (list == NULL || size == NULL) {
        return linked_list_peek_head(list, size);
    }

    xSemaphoreTake_ExpectAndReturn(list->mutex, portMAX_DELAY, pdTRUE);

    if (list->count == 0) {
        xSemaphoreGive_ExpectAndReturn(list->mutex, pdTRUE);
        return linked_list_peek_head(list, size);
    }

    xSemaphoreGive_ExpectAndReturn(list->mutex, pdTRUE);
    return linked_list_peek_head(list, size);
}

// Mock helper for linked_list_peek_tail
void *mock_linked_list_peek_tail(linked_list_t *list, size_t *size)
{
    if (list == NULL || size == NULL) {
        return linked_list_peek_tail(list, size);
    }

    xSemaphoreTake_ExpectAndReturn(list->mutex, portMAX_DELAY, pdTRUE);

    if (list->count == 0) {
        xSemaphoreGive_ExpectAndReturn(list->mutex, pdTRUE);
        return linked_list_peek_tail(list, size);
    }

    xSemaphoreGive_ExpectAndReturn(list->mutex, pdTRUE);
    return linked_list_peek_tail(list, size);
}

// Mock helper for linked_list_count
size_t mock_linked_list_count(linked_list_t *list)
{
    if (list == NULL) {
        return linked_list_count(list);
    }

    xSemaphoreTake_ExpectAndReturn(list->mutex, portMAX_DELAY, pdTRUE);
    xSemaphoreGive_ExpectAndReturn(list->mutex, pdTRUE);
    return linked_list_count(list);
}

// Mock helper for linked_list_destroy
void mock_linked_list_destroy(linked_list_t *list)
{
    if (list == NULL) {
        linked_list_destroy(list);
        return;
    }

    while (list->count > 0) {
        size_t size;
        mock_linked_list_pop_head(list, &size);
    }

    if (list->mutex != NULL) {
        vSemaphoreDelete_Expect(list->mutex);
    }

    linked_list_destroy(list);
}

void test_linked_list_create(void)
{
    linked_list_t list;
    mock_linked_list_create(&list, 100);
    TEST_ASSERT_EQUAL(100, list.max_size);
    TEST_ASSERT_NULL(list.head);
    TEST_ASSERT_NULL(list.tail);
    TEST_ASSERT_EQUAL(0, list.count);
    TEST_ASSERT_NOT_NULL(list.mutex);

    mock_linked_list_destroy(&list);
}

void test_linked_list_create_failure(void)
{
    TEST_ASSERT_FALSE(linked_list_create(NULL, 10));
    TEST_ASSERT_FALSE(linked_list_create(&test_list, 0));
}

void test_linked_list_enqueue_single(void)
{
    linked_list_t list;
    mock_linked_list_create(&list, 10);

    test_item_t item = {42, "test_item"};
    TEST_ASSERT_TRUE(mock_linked_list_enqueue(&list, &item, sizeof(item)));
    TEST_ASSERT_EQUAL(1, list.count);

    // Clean up: need to free the enqueued node
    mock_linked_list_destroy(&list);
}

void test_linked_list_enqueue_multiple(void)
{
    linked_list_t list;
    mock_linked_list_create(&list, 10);

    test_item_t item1 = {1, "first"};
    test_item_t item2 = {2, "second"};
    test_item_t item3 = {3, "third"};

    TEST_ASSERT_TRUE(mock_linked_list_enqueue(&list, &item1, sizeof(item1)));
    TEST_ASSERT_TRUE(mock_linked_list_enqueue(&list, &item2, sizeof(item2)));
    TEST_ASSERT_TRUE(mock_linked_list_enqueue(&list, &item3, sizeof(item3)));

    TEST_ASSERT_EQUAL(3, list.count);
    TEST_ASSERT_NOT_NULL(list.head);
    TEST_ASSERT_NOT_NULL(list.tail);

    mock_linked_list_destroy(&list);
}

void test_linked_list_enqueue_failure_null_params(void)
{
    linked_list_t list;
    mock_linked_list_create(&list, 10);

    test_item_t item = {42, "test"};

    // NULL list
    TEST_ASSERT_FALSE(linked_list_enqueue(NULL, &item, sizeof(item)));

    // NULL data
    TEST_ASSERT_FALSE(linked_list_enqueue(&list, NULL, sizeof(item)));

    // Zero size
    TEST_ASSERT_FALSE(linked_list_enqueue(&list, &item, 0));

    mock_linked_list_destroy(&list);
}

void test_linked_list_enqueue_failure_full(void)
{
    linked_list_t list;
    mock_linked_list_create(&list, 2);

    test_item_t item1 = {1, "first"};
    test_item_t item2 = {2, "second"};
    test_item_t item3 = {3, "third"};

    // Fill the list
    TEST_ASSERT_TRUE(mock_linked_list_enqueue(&list, &item1, sizeof(item1)));
    TEST_ASSERT_TRUE(mock_linked_list_enqueue(&list, &item2, sizeof(item2)));

    // List is full, should fail
    xSemaphoreTake_ExpectAndReturn(list.mutex, portMAX_DELAY, pdTRUE);
    xSemaphoreGive_ExpectAndReturn(list.mutex, pdTRUE);
    TEST_ASSERT_FALSE(linked_list_enqueue(&list, &item3, sizeof(item3)));

    mock_linked_list_destroy(&list);
}

void test_linked_list_enqueue_failure_malloc(void)
{
    linked_list_t list;
    mock_linked_list_create(&list, 10);

    test_item_t item = {42, "test"};

    // Set up expectations: semaphore take succeeds, malloc fails
    xSemaphoreTake_ExpectAndReturn(list.mutex, portMAX_DELAY, pdTRUE);
    heap_caps_malloc_ExpectAndReturn(sizeof(linked_list_node_t), MALLOC_CAP_SPIRAM, NULL);
    xSemaphoreGive_ExpectAndReturn(list.mutex, pdTRUE);

    TEST_ASSERT_FALSE(linked_list_enqueue(&list, &item, sizeof(item)));

    mock_linked_list_destroy(&list);
}

void test_linked_list_peek_head(void)
{
    linked_list_t list;
    mock_linked_list_create(&list, 10);

    test_item_t item1 = {100, "first"};
    test_item_t item2 = {200, "second"};

    TEST_ASSERT_TRUE(mock_linked_list_enqueue(&list, &item1, sizeof(item1)));
    TEST_ASSERT_TRUE(mock_linked_list_enqueue(&list, &item2, sizeof(item2)));

    size_t size;
    void *data = mock_linked_list_peek_head(&list, &size);

    TEST_ASSERT_NOT_NULL(data);
    TEST_ASSERT_EQUAL(sizeof(item1), size);
    TEST_ASSERT_EQUAL(&item1, data);  // Should return the pointer we enqueued
    TEST_ASSERT_EQUAL(2, list.count);  // Peek doesn't remove

    mock_linked_list_destroy(&list);
}

void test_linked_list_peek_head_empty(void)
{
    linked_list_t list;
    mock_linked_list_create(&list, 10);

    size_t size;
    void *data = mock_linked_list_peek_head(&list, &size);

    TEST_ASSERT_NULL(data);
    TEST_ASSERT_EQUAL(0, list.count);

    mock_linked_list_destroy(&list);
}

void test_linked_list_peek_head_null_params(void)
{
    linked_list_t list;
    mock_linked_list_create(&list, 10);

    size_t size;

    // NULL list
    TEST_ASSERT_NULL(linked_list_peek_head(NULL, &size));

    // NULL size
    TEST_ASSERT_NULL(linked_list_peek_head(&list, NULL));

    mock_linked_list_destroy(&list);
}

void test_linked_list_peek_tail(void)
{
    linked_list_t list;
    mock_linked_list_create(&list, 10);

    test_item_t item1 = {100, "first"};
    test_item_t item2 = {200, "second"};

    TEST_ASSERT_TRUE(mock_linked_list_enqueue(&list, &item1, sizeof(item1)));
    TEST_ASSERT_TRUE(mock_linked_list_enqueue(&list, &item2, sizeof(item2)));

    size_t size;
    void *data = mock_linked_list_peek_tail(&list, &size);

    TEST_ASSERT_NOT_NULL(data);
    TEST_ASSERT_EQUAL(sizeof(item2), size);
    TEST_ASSERT_EQUAL(&item2, data);  // Should return the last item
    TEST_ASSERT_EQUAL(2, list.count);  // Peek doesn't remove

    mock_linked_list_destroy(&list);
}

void test_linked_list_peek_tail_empty(void)
{
    linked_list_t list;
    mock_linked_list_create(&list, 10);

    size_t size;
    void *data = mock_linked_list_peek_tail(&list, &size);

    TEST_ASSERT_NULL(data);
    TEST_ASSERT_EQUAL(0, list.count);

    mock_linked_list_destroy(&list);
}

void test_linked_list_peek_tail_null_params(void)
{
    linked_list_t list;
    mock_linked_list_create(&list, 10);

    size_t size;

    // NULL list
    TEST_ASSERT_NULL(linked_list_peek_tail(NULL, &size));

    // NULL size
    TEST_ASSERT_NULL(linked_list_peek_tail(&list, NULL));

    mock_linked_list_destroy(&list);
}

void test_linked_list_pop_head(void)
{
    linked_list_t list;
    mock_linked_list_create(&list, 10);

    test_item_t item1 = {100, "first"};
    test_item_t item2 = {200, "second"};

    TEST_ASSERT_TRUE(mock_linked_list_enqueue(&list, &item1, sizeof(item1)));
    TEST_ASSERT_TRUE(mock_linked_list_enqueue(&list, &item2, sizeof(item2)));

    size_t size;
    void *data = mock_linked_list_pop_head(&list, &size);

    TEST_ASSERT_NOT_NULL(data);
    TEST_ASSERT_EQUAL(sizeof(item1), size);
    TEST_ASSERT_EQUAL(&item1, data);
    TEST_ASSERT_EQUAL(1, list.count);  // Pop removes the item

    mock_linked_list_destroy(&list);
}

void test_linked_list_pop_head_single(void)
{
    linked_list_t list;
    mock_linked_list_create(&list, 10);

    test_item_t item = {42, "single"};
    TEST_ASSERT_TRUE(mock_linked_list_enqueue(&list, &item, sizeof(item)));

    size_t size;
    void *data = mock_linked_list_pop_head(&list, &size);

    TEST_ASSERT_NOT_NULL(data);
    TEST_ASSERT_EQUAL(&item, data);
    TEST_ASSERT_EQUAL(0, list.count);
    TEST_ASSERT_NULL(list.head);
    TEST_ASSERT_NULL(list.tail);

    mock_linked_list_destroy(&list);
}

void test_linked_list_pop_head_empty(void)
{
    linked_list_t list;
    mock_linked_list_create(&list, 10);

    size_t size;
    void *data = mock_linked_list_pop_head(&list, &size);

    TEST_ASSERT_NULL(data);
    TEST_ASSERT_EQUAL(0, list.count);

    mock_linked_list_destroy(&list);
}

void test_linked_list_pop_head_null_params(void)
{
    linked_list_t list;
    mock_linked_list_create(&list, 10);

    size_t size;

    // NULL list
    TEST_ASSERT_NULL(linked_list_pop_head(NULL, &size));

    // NULL size
    TEST_ASSERT_NULL(linked_list_pop_head(&list, NULL));

    mock_linked_list_destroy(&list);
}

void test_linked_list_pop_tail(void)
{
    linked_list_t list;
    mock_linked_list_create(&list, 10);

    test_item_t item1 = {100, "first"};
    test_item_t item2 = {200, "second"};
    test_item_t item3 = {300, "third"};

    TEST_ASSERT_TRUE(mock_linked_list_enqueue(&list, &item1, sizeof(item1)));
    TEST_ASSERT_TRUE(mock_linked_list_enqueue(&list, &item2, sizeof(item2)));
    TEST_ASSERT_TRUE(mock_linked_list_enqueue(&list, &item3, sizeof(item3)));

    size_t size;
    void *data = mock_linked_list_pop_tail(&list, &size);

    TEST_ASSERT_NOT_NULL(data);
    TEST_ASSERT_EQUAL(sizeof(item3), size);
    TEST_ASSERT_EQUAL(&item3, data);  // Pop tail removes last item
    TEST_ASSERT_EQUAL(2, list.count);

    mock_linked_list_destroy(&list);
}

void test_linked_list_pop_tail_single(void)
{
    linked_list_t list;
    mock_linked_list_create(&list, 10);

    test_item_t item = {42, "single"};
    TEST_ASSERT_TRUE(mock_linked_list_enqueue(&list, &item, sizeof(item)));

    size_t size;
    void *data = mock_linked_list_pop_tail(&list, &size);

    TEST_ASSERT_NOT_NULL(data);
    TEST_ASSERT_EQUAL(&item, data);
    TEST_ASSERT_EQUAL(0, list.count);
    TEST_ASSERT_NULL(list.head);
    TEST_ASSERT_NULL(list.tail);

    mock_linked_list_destroy(&list);
}

void test_linked_list_pop_tail_empty(void)
{
    linked_list_t list;
    mock_linked_list_create(&list, 10);

    size_t size;
    void *data = mock_linked_list_pop_tail(&list, &size);

    TEST_ASSERT_NULL(data);
    TEST_ASSERT_EQUAL(0, list.count);

    mock_linked_list_destroy(&list);
}

void test_linked_list_pop_tail_null_params(void)
{
    linked_list_t list;
    mock_linked_list_create(&list, 10);

    size_t size;

    // NULL list
    TEST_ASSERT_NULL(linked_list_pop_tail(NULL, &size));

    // NULL size
    TEST_ASSERT_NULL(linked_list_pop_tail(&list, NULL));

    mock_linked_list_destroy(&list);
}

void test_linked_list_count(void)
{
    linked_list_t list;
    mock_linked_list_create(&list, 10);

    TEST_ASSERT_EQUAL(0, mock_linked_list_count(&list));

    test_item_t item1 = {1, "first"};
    test_item_t item2 = {2, "second"};
    test_item_t item3 = {3, "third"};

    TEST_ASSERT_TRUE(mock_linked_list_enqueue(&list, &item1, sizeof(item1)));
    TEST_ASSERT_EQUAL(1, mock_linked_list_count(&list));

    TEST_ASSERT_TRUE(mock_linked_list_enqueue(&list, &item2, sizeof(item2)));
    TEST_ASSERT_EQUAL(2, mock_linked_list_count(&list));

    TEST_ASSERT_TRUE(mock_linked_list_enqueue(&list, &item3, sizeof(item3)));
    TEST_ASSERT_EQUAL(3, mock_linked_list_count(&list));

    mock_linked_list_destroy(&list);
}

void test_linked_list_count_null(void)
{
    TEST_ASSERT_EQUAL(0, linked_list_count(NULL));
}

void test_linked_list_destroy_null(void)
{
    // Should not crash
    linked_list_destroy(NULL);
}

void test_linked_list_fifo_order(void)
{
    linked_list_t list;
    mock_linked_list_create(&list, 10);

    test_item_t item1 = {1, "first"};
    test_item_t item2 = {2, "second"};
    test_item_t item3 = {3, "third"};

    // Enqueue three items
    TEST_ASSERT_TRUE(mock_linked_list_enqueue(&list, &item1, sizeof(item1)));
    TEST_ASSERT_TRUE(mock_linked_list_enqueue(&list, &item2, sizeof(item2)));
    TEST_ASSERT_TRUE(mock_linked_list_enqueue(&list, &item3, sizeof(item3)));

    // Pop all - should be FIFO order
    size_t size;
    void *data1 = mock_linked_list_pop_head(&list, &size);
    void *data2 = mock_linked_list_pop_head(&list, &size);
    void *data3 = mock_linked_list_pop_head(&list, &size);

    TEST_ASSERT_EQUAL(&item1, data1);
    TEST_ASSERT_EQUAL(&item2, data2);
    TEST_ASSERT_EQUAL(&item3, data3);
    TEST_ASSERT_EQUAL(0, list.count);

    mock_linked_list_destroy(&list);
}

void test_linked_list_all(void)
{
    printf("Running linked list tests...\n");
    
    // Create tests
    RUN_TEST(test_linked_list_create);
    RUN_TEST(test_linked_list_create_failure);
    
    // Enqueue tests
    RUN_TEST(test_linked_list_enqueue_single);
    RUN_TEST(test_linked_list_enqueue_multiple);
    RUN_TEST(test_linked_list_enqueue_failure_null_params);
    RUN_TEST(test_linked_list_enqueue_failure_full);
    RUN_TEST(test_linked_list_enqueue_failure_malloc);
    
    // Peek tests
    RUN_TEST(test_linked_list_peek_head);
    RUN_TEST(test_linked_list_peek_head_empty);
    RUN_TEST(test_linked_list_peek_head_null_params);
    RUN_TEST(test_linked_list_peek_tail);
    RUN_TEST(test_linked_list_peek_tail_empty);
    RUN_TEST(test_linked_list_peek_tail_null_params);
    
    // Pop tests
    RUN_TEST(test_linked_list_pop_head);
    RUN_TEST(test_linked_list_pop_head_single);
    RUN_TEST(test_linked_list_pop_head_empty);
    RUN_TEST(test_linked_list_pop_head_null_params);
    RUN_TEST(test_linked_list_pop_tail);
    RUN_TEST(test_linked_list_pop_tail_single);
    RUN_TEST(test_linked_list_pop_tail_empty);
    RUN_TEST(test_linked_list_pop_tail_null_params);
    
    // Count tests
    RUN_TEST(test_linked_list_count);
    RUN_TEST(test_linked_list_count_null);
    
    // Destroy tests
    RUN_TEST(test_linked_list_destroy_null);
    
    // Integration tests
    RUN_TEST(test_linked_list_fifo_order);
    
    printf("Linked list tests completed\n");
}
