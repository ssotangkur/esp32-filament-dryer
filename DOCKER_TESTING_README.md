# ESP32 Firmware Unit Testing with CMock

This guide explains how to run automated unit tests for your ESP32 filament dryer firmware using the Unity testing framework with CMock.

## Running Unit Tests

```batch
run_unit_tests.bat
```

Or manually:
```bash
docker-compose run --rm unit-tests
```

## Writing Tests with CMock

### Basic Test Structure

```c
#include "unity.h"

void test_component_basic_functionality(void) {
    TEST_ASSERT_EQUAL(expected, actual);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_component_basic_functionality);
    return UNITY_END();
}
```

### Testing Real Implementations

When testing real implementations (not fully mocked), include mock headers BEFORE the real header:

```c
#include "unity.h"

// Include CMock-generated mocks FIRST
#include "Mockmock_esp_heap_caps.h"
#include "Mockmock_semphr.h"

// Include real implementation AFTER mocks
#include "circular_buffer.h"
```

This allows CMock to intercept ESP-IDF/FreeRTOS function calls while using the real implementation.

### Using Real malloc() in Tests

To use real memory allocation:

```c
void *mock_circular_buffer_init_success(circular_buffer_t *cb, size_t element_size, size_t buffer_size)
{
    // Allocate REAL memory with malloc()
    size_t mem_size = element_size * buffer_size;
    void *mock_buffer = malloc(mem_size);
    
    // Tell CMock to return this real memory address
    heap_caps_malloc_ExpectAndReturn(mem_size, MALLOC_CAP_SPIRAM, mock_buffer);
    xSemaphoreCreateMutex_ExpectAndReturn(SEMAPHORE_1);
    
    cb->mutex = SEMAPHORE_1;
    TEST_ASSERT_TRUE(circular_buffer_init(cb, element_size, buffer_size));
    return mock_buffer;
}
```

### Wrapper Functions That Call Real Functions

**What are mock wrappers?**

Mock wrapper functions are helper functions that encapsulate CMock expectation setup and then call the real function. They keep test code clean and reusable by moving mock setup logic out of individual test functions.

**Why use mock wrappers instead of calling mock macros directly in tests?**

1. **Cleaner tests** - Test functions focus on *what* is being tested, not *how* mocks are set up
2. **Reusable setup** - Common mock sequences are written once, used many times
3. **Easier maintenance** - If function behavior changes, update the wrapper, not every test
4. **Consistent expectations** - Reduces risk of missing or incorrect mock setup in tests

**Basic wrapper pattern:**

```c
// Wrapper sets up expectations AND calls real function
bool mock_circular_buffer_count(circular_buffer_t *cb)
{
    xSemaphoreTake_ExpectAndReturn(cb->mutex, portMAX_DELAY, pdTRUE);
    xSemaphoreGive_ExpectAndReturn(cb->mutex, pdTRUE);
    return circular_buffer_count(cb);  // Call REAL function
}

// In tests - ONLY use the mock wrapper
void test_circular_buffer_push_pop(void)
{
    mock_circular_buffer_push(&test_buf, &item1);
    TEST_ASSERT_EQUAL(1, mock_circular_buffer_count(&test_buf));
}
```

**Creating wrappers that follow branch logic:**

When creating a mock wrapper, you must replicate the branch logic of the real function and set up expectations for each possible execution path:

```c
void *mock_linked_list_pop_head(linked_list_t *list, size_t *size)
{
    // Step 1: Handle NULL parameters (same as real function)
    if (list == NULL || size == NULL) {
        return linked_list_pop_head(list, size);  // No mocks needed - function returns early
    }

    // Step 2: Set up expectation for semaphore take (always happens)
    xSemaphoreTake_ExpectAndReturn(list->mutex, portMAX_DELAY, pdTRUE);

    // Step 3: Handle empty list branch (function checks list->count == 0)
    if (list->count == 0) {
        xSemaphoreGive_ExpectAndReturn(list->mutex, pdTRUE);  // Give happens before early return
        return linked_list_pop_head(list, size);
    }

    // Step 4: Normal path - node will be freed
    heap_caps_free_ExpectAnyArgs();
    xSemaphoreGive_ExpectAndReturn(list->mutex, pdTRUE);
    return linked_list_pop_head(list, size);
}
```

**Key principles for wrapper creation:**

1. **Mirror the function's control flow** - If the real function has `if (count == 0) { return NULL; }`, your wrapper must set up mocks for that path
2. **Set expectations before calling the real function** - CMock needs expectations set up before the real function runs
3. **Handle error paths** - NULL parameters, empty buffers, full buffers, etc.
4. **Use `ExpectAnyArgs()` when the specific value doesn't matter** - e.g., for `free()` calls

**Example: Wrapper for a function with multiple failure modes:**

```c
bool mock_linked_list_enqueue(linked_list_t *list, const void *data, size_t size)
{
    // Handle parameter validation (real function returns false for these)
    if (list == NULL || data == NULL || size == 0) {
        return linked_list_enqueue(list, data, size);
    }

    // Normal path: semaphore take succeeds
    xSemaphoreTake_ExpectAndReturn(list->mutex, portMAX_DELAY, pdTRUE);

    // Malloc is called (provide real memory so function can dereference)
    linked_list_node_t *real_node = malloc(sizeof(linked_list_node_t));
    heap_caps_malloc_ExpectAndReturn(sizeof(linked_list_node_t), MALLOC_CAP_SPIRAM, real_node);

    // Semaphore give (always called after successful operations)
    xSemaphoreGive_ExpectAndReturn(list->mutex, pdTRUE);

    return linked_list_enqueue(list, data, size);
}

// For testing malloc failure, set up expectations directly in the test:
void test_enqueue_malloc_failure(void)
{
    linked_list_t list;
    mock_linked_list_create(&list, 10);

    xSemaphoreTake_ExpectAndReturn(list.mutex, portMAX_DELAY, pdTRUE);
    heap_caps_malloc_ExpectAndReturn(sizeof(linked_list_node_t), MALLOC_CAP_SPIRAM, NULL);
    xSemaphoreGive_ExpectAndReturn(list.mutex, pdTRUE);

    test_item_t item = {42, "test"};
    TEST_ASSERT_FALSE(linked_list_enqueue(&list, &item, sizeof(item)));

    mock_linked_list_destroy(&list);
}
```

### CMake Configuration

```cmake
set(SOURCES
    main/test_component.c
    ${MOCK_SOURCES}
    /project/main/real_component.c  # Real implementation
)
```

### Patterns to Avoid

1. **DON'T call Mockmock_*_Init() or Mockmock_*_Verify()** - CMock handles this automatically
2. **DON'T use internal functions** like `*_CMockExpectAndReturn` - Use public `_ExpectAndReturn` macros
3. **DON'T mix mock wrappers and real function calls** - Use only the mock wrapper

## Troubleshooting

- **Build fails**: Check mock headers are included correctly
- **Test fails**: Verify expectations match actual function calls
- **Missing mocks**: Add `mock_*.h` to `mock_headers/` and regenerate

---

**Status**: Unit testing framework operational!
