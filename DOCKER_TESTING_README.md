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

The recommended pattern:

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
