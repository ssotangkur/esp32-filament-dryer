---
name: esp32-testing-patterns
description: ESP32 unit testing patterns with CMock and Docker for ESP-IDF projects
---

## ESP32 Unit Testing Rules

### Overview
This project uses comprehensive unit testing for ESP32 applications with ESP-IDF/FreeRTOS integration. Tests run in Docker containers with CMock for automated mock generation, including advanced ignore plugins for flexible mocking.

## Mock Header Architecture

### File Structure Pattern
```
docker_tests/unit_tests/
├── CMakeLists.txt                    # Build configuration with global variables
├── cmock.yml                         # CMock plugin configuration
├── cmock_globals.c                   # Auto-generated global variables for ignore plugins
├── mock_headers/                     # Mock header definitions
│   ├── [component]_wrapper.h         # Wrapper headers matching ESP-IDF paths
│   ├── mock_[component].h            # CMock mock function definitions
│   └── freertos/                     # FreeRTOS-specific headers
│       ├── FreeRTOS.h               # Minimal FreeRTOS types only
│       └── semphr.h                 # FreeRTOS function prototypes
├── mocks/                           # Generated mock files (auto-generated)
│   └── Mockmock_[component].c,h
└── main/                            # Test source files
    ├── test_[component].c           # Component tests with wrapper functions
    └── [component]_mocks.c          # Mock wrapper functions (optional)
```

### FreeRTOS Mocking Strategy
- **FreeRTOS.h**: Define minimal essential types (`SemaphoreHandle_t`, `pdTRUE`, etc.)
- **semphr.h**: Function prototypes for CMock mocking
- **mock_semphr.h**: CMock-generated mock functions
- Avoid full ESP-IDF FreeRTOS headers to prevent dependency issues

## Test Organization

### Test File Structure
```c
#include "unity.h"
#include "Mockmock_component.h"      // CMock-generated mocks
#include "../../main/source_file.c"  // Source under test

// Test data structures
typedef struct {
    int value;
    char name[32];
} test_data_t;

// Global test fixtures
static test_data_t test_data;
static uint8_t mock_buffer[BUFFER_SIZE];

// Test functions
void setUp(void) { /* Initialize mocks */ }
void tearDown(void) { /* Cleanup */ }
void test_functionality(void) { /* Test logic */ }
```

### CMock Integration with Ignore Plugins
```c
// Initialize mocks at test start
void setUp(void) {
    Mockmock_component_Init();
    Mockanother_component_Init();
}

// Clean macros (recommended for new code)
heap_caps_malloc_ExpectAndReturn(size, caps, mock_buffer);
xSemaphoreTake_ExpectAndReturn(handle, portMAX_DELAY, pdTRUE);

// Ignore plugins for flexible argument matching
heap_caps_malloc_IgnoreAndReturn(mock_buffer);           // Ignore all arguments
xSemaphoreTake_IgnoreArg_handle();                       // Ignore specific argument
adc_oneshot_read_ExpectAnyArgsAndReturn(ADC_ATTEN_DB_0, ESP_OK); // Any args OK

// Execute test
// CMock automatically verifies expectations
```

### Using Ignore Plugins

Ignore plugins provide flexible argument matching when you don't need to verify specific parameter values:

```c
// Good reasons to use Ignore macros:
// 1. When testing error handling and arguments don't matter
// 2. When mocking utility functions that vary by context
// 3. When focusing on return values rather than input validation

// Example: Testing memory allocation failure
void test_memory_allocation_failure(void) {
    // Ignore all arguments - we only care about the failure return
    heap_caps_malloc_IgnoreAndReturn(NULL);

    // Test code that should handle NULL return gracefully
    TEST_ASSERT_FALSE(some_function_that_allocates_memory());
}

// Example: Ignoring handle parameters in semaphore operations
void test_semaphore_timeout(void) {
    // Ignore the handle parameter - we know it's valid
    xSemaphoreTake_IgnoreArg_handle();
    xSemaphoreTake_ReturnThruPtr_wait_time(&actual_wait_time);

    // Test timeout behavior
    esp_err_t result = wait_for_semaphore_with_timeout(test_handle, 1000);
    TEST_ASSERT_EQUAL(ESP_ERR_TIMEOUT, result);
}
```

## ESP-IDF Component Testing

### Memory Management Testing
- Mock `heap_caps_malloc()` and `heap_caps_free()`
- Use real memory buffers for testing (avoid NULL pointer issues)

### Thread Safety Testing
- Mock FreeRTOS semaphore functions
- Test mutex acquisition/release patterns
- Verify proper semaphore cleanup

### ADC/Temperature Testing
- Mock ADC calibration and reading functions
- Test sensor error conditions (disconnected, short circuit)
- Verify proper handle management

## Test Execution

### Docker-Based Testing
```powershell
# Run all tests (Windows/PowerShell)
powershell -Command "cd docker_tests; .\run_unit_tests.bat"

# Or via Docker directly (Windows/PowerShell)
docker run --rm -v ${pwd}:/project espressif/idf:v5.5.1 `
  bash -c "cd unit_tests && ./run_unit_tests.sh"
```

### Mock Generation
```powershell
# Generate mocks from mock_headers/mock_*.h files (Windows/PowerShell)
powershell -Command "cd docker_tests; .\generate_mocks.bat"
```

## Best Practices

### Test Coverage
- **Initialization**: Valid/invalid parameters, memory allocation
- **Normal operations**: Basic functionality with valid inputs
- **Edge cases**: Buffer overflow, empty buffers, boundary conditions
- **Error handling**: NULL pointers, invalid states, resource failures
- **Thread safety**: Semaphore operations, concurrent access

### Mock Management
- Initialize all mocks in `setUp()`
- Set expectations before operations under test
- Use realistic return values (real memory buffers, valid handles)
- Clean up resources in `tearDown()`

### Test Naming
- `test_component_functionality()` - Basic operations
- `test_component_edge_cases()` - Boundary conditions
- `test_component_error_handling()` - Error scenarios
- `test_component_thread_safety()` - Concurrency testing

## Common Patterns

### Buffer Testing Pattern
```c
// Setup
static uint8_t mock_buffer[BUFFER_SIZE];
circular_buffer_t test_buf;

// Test
heap_caps_malloc_ExpectAndReturn(BUFFER_SIZE, MALLOC_CAP_SPIRAM, mock_buffer);
xSemaphoreCreateMutex_ExpectAndReturn((SemaphoreHandle_t)0x2000);
// ... more expectations

TEST_ASSERT_TRUE(circular_buffer_init(&test_buf, sizeof(element_t), CAPACITY));
// Test operations...
circular_buffer_free(&test_buf);
```

### Semaphore Testing Pattern
```c
// Mock semaphore operations (use clean macros when possible)
xSemaphoreTake_ExpectAndReturn(handle, portMAX_DELAY, pdTRUE);
xSemaphoreGive_ExpectAndReturn(handle, pdTRUE);
vSemaphoreDelete_Expect(handle);

// For call counting scenarios, use internal macros:
// xSemaphoreTake_CMockExpectAndReturn(call_count, handle, portMAX_DELAY, pdTRUE);
// xSemaphoreGive_CMockExpectAndReturn(call_count + 1, handle, pdTRUE);
// vSemaphoreDelete_CMockExpect(call_count + 2, handle);
```

## Error Prevention

### Avoid Common Issues
- **Don't include real ESP-IDF headers directly** - Use minimal mock headers
- **Don't use NULL buffers in mocks** - Use real allocated memory
- **Don't forget mock initialization** - Always call `Mock*_Init()` in `setUp()`
- **Don't use internal _CMock macros** - Use clean `ExpectAndReturn()` macros instead
- **Don't mix mock expectations** - Set all expectations before test execution
- **Don't rely on exact call counts** - Use patterns for repeated operations

### Debugging Tests
- Check mock expectation failures - CMock provides detailed error messages
- Verify header includes match mock generation
- Ensure Docker container has proper ESP-IDF environment
- Use `UNITY_PRINT()` for debugging output in containerized tests