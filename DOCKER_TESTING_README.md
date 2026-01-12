# ESP32 Firmware Unit Testing with CMock

This guide explains how to run automated unit tests for your ESP32 filament dryer firmware using the Unity testing framework with CMock for automatic mock generation, including advanced ignore plugins for flexible mocking.

## Overview

Your project includes a comprehensive unit testing framework that allows testing individual components in isolation without hardware dependencies. The framework features wrapper functions for clean test code and ignore plugins for flexible argument matching.

## Testing Framework Structure

```
docker_tests/unit_tests/
├── CMakeLists.txt                    # Build configuration with global variables
├── cmock.yml                         # CMock plugin configuration
├── cmock_globals.c                   # Auto-generated global variables for ignore plugins
├── run_unit_tests.sh                # Test runner script
├── generate_mocks.sh               # Shared mock generation script
├── mock_headers/                   # Mock header definitions
│   ├── mock_*.h                    # CMock mock function definitions
│   └── freertos/                   # FreeRTOS-specific headers
├── mocks/                          # Generated mock files (auto-generated)
│   └── Mockmock_*.c,h
└── main/                           # Test source files
    ├── test_*.c                    # Component tests with wrapper functions
    └── *_mocks.c                   # Mock wrapper functions (optional)
```

## Running Unit Tests

### Prerequisites
- Docker Desktop installed and running
- Windows 10/11 with WSL2 (recommended) or Hyper-V

### Quick Start
```batch
# Run all unit tests in Docker (works from any directory)
run_unit_tests.bat
```

**Note**: The `run_unit_tests.bat` script can be executed from any directory - it automatically finds the required files and Docker configuration.

### Manual Docker Commands
```bash
# Build the Docker image
docker-compose build

# Run unit tests
docker-compose run --rm unit-tests
```

### Manual Testing
```bash
# Set up ESP-IDF environment
idf.py --preview set-target esp32

# Build and run tests
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor  # Or use your preferred flashing method
```

### Mock Management

#### Directory Structure
```
docker_tests/unit_tests/
├── mock_headers/           # Mock header definitions (mock_*.h files)
│   └── mock_esp_adc.h     # Example: ADC hardware interface mocks
├── mocks/                  # Generated mock files (auto-generated)
│   ├── Mockmock_esp_adc.c
│   └── Mockmock_esp_adc.h
├── generate_mocks.sh       # Shared mock generation script
└── main/                   # Test files
```

#### Adding New Mocks
1. Create a new header file in `mock_headers/` with `mock_` prefix:
   ```bash
   # Example: create mock_wifi.h
   ```

2. Generate mocks (two options):

   **Option A - Standalone generation:**
   ```batch
   cd docker_tests
   generate_mocks.bat
   ```

   **Option B - As part of full test run:**
   ```batch
   cd docker_tests
   run_unit_tests.bat  # Automatically generates mocks first
   ```

3. The shared `generate_mocks.sh` script automatically discovers all `mock_*.h` files and generates corresponding mocks

#### How It Works
- **Shared Script**: `generate_mocks.sh` is used by both standalone generation and full test runs
- **Convention-Based**: Files with `mock_` prefix in `mock_headers/` are automatically processed
- **Docker-Powered**: Uses the same ESP-IDF Docker environment for consistency
- **Automatic Inclusion**: CMake finds and includes all generated `.c` files from `mocks/`

## Test Framework Features

### Unity Testing Framework
- Lightweight C testing framework
- Automatic test discovery and execution
- Detailed failure reporting with line numbers
- Support for setup/teardown functions

### CMock Mock Generation
- Automatic mock generation from header files
- Function call verification and expectations
- Return value control for mocked functions
- Parameter validation and capture

### Test Organization
- Each test file focuses on a single component
- Clear naming convention: `test_<component>.c`
- Comprehensive coverage of edge cases
- Separation of concerns between units

## Writing New Tests

### Basic Test Structure
```c
#include "unity.h"
#include "component_under_test.h"

// Test setup (optional)
void setUp(void) {
    // Initialize test fixtures
}

// Test teardown (optional)
void tearDown(void) {
    // Clean up after each test
}

// Individual test functions
void test_component_basic_functionality(void) {
    // Arrange
    // Act
    // Assert
    TEST_ASSERT_EQUAL(expected_value, actual_value);
}

// Main test runner
int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_component_basic_functionality);
    return UNITY_END();
}
```

### Using CMock Mocks
```c
#include "unity.h"
#include "cmock.h"
#include "mock_hardware_driver.h"

// Create mock for hardware_driver.h
void test_with_mocked_hardware(void) {
    // Expect hardware call and define return value
    hardware_read_ExpectAndReturn(0x1234);

    // Test code that calls hardware
    uint16_t result = read_sensor_value();

    // Verify expectations
    TEST_ASSERT_EQUAL(0x1234, result);
}
```

### CMock Ignore Plugins

The framework supports advanced ignore plugins for flexible argument matching:

```c
#include "unity.h"
#include "Mockmock_component.h"

// Ignore all arguments - useful for functions where arguments don't matter
heap_caps_malloc_IgnoreAndReturn(mock_buffer);

// Ignore specific arguments - useful for handle-based functions
xSemaphoreTake_IgnoreArg_handle();

// Accept any arguments - useful for error-tolerant functions
adc_oneshot_read_ExpectAnyArgsAndReturn(ADC_ATTEN_DB_0, ESP_OK);

// Clean macros (recommended for new code)
adc_oneshot_read_ExpectAndReturn(handle, chan, out_raw, ESP_OK);

// Internal macros with call counting (for complex scenarios)
adc_oneshot_read_CMockExpectAndReturn(1, handle, chan, out_raw, ESP_OK);
```

### Wrapper Functions for Clean Tests

For improved readability, use wrapper functions to encapsulate complex mock expectations:

```c
// In test file or separate mock wrapper file
void mock_circular_buffer_init_success(size_t element_size, size_t buffer_size) {
    // Set up all necessary mocks for successful initialization
    heap_caps_malloc_CMockExpectAndReturn(1, buffer_size * element_size, MALLOC_CAP_SPIRAM, mock_buffer);
    xSemaphoreCreateMutex_CMockExpectAndReturn(2, (SemaphoreHandle_t)0x2000);
    // ... additional expectations
}

// In test functions - much cleaner!
void test_circular_buffer_init(void) {
    mock_circular_buffer_init_success(sizeof(element_t), CAPACITY);

    TEST_ASSERT_TRUE(circular_buffer_init(&test_buf, sizeof(element_t), CAPACITY));
}
```

## Test Execution Output

When tests run successfully, you'll see output like:
```
test_circular_buffer.c:23:test_buffer_initialization:PASS
test_circular_buffer.c:45:test_buffer_overflow:PASS
test_temperature.c:12:test_valid_conversion:PASS

-----------------------
7 Tests 0 Failures 0 Ignored
OK
```

## Integration with Development Workflow

1. **Write tests alongside code** - Test-driven development
2. **Run tests before committing** - Prevent regressions
3. **Include in CI/CD** - Automated quality gates
4. **Debug failures quickly** - Isolated component testing

## Troubleshooting

### Build Issues
- Ensure ESP-IDF environment is activated
- Check CMake version compatibility
- Verify all test dependencies are included

### Test Failures
- Check test expectations vs. actual behavior
- Verify mock setup is correct
- Review component interface changes

### Missing Mocks
- **Automatic generation**: Add `mock_*.h` files to `mock_headers/` and run `generate_mocks.bat`
- **Manual generation**: Use the shared script: `docker run --rm -v "$(pwd):/project" -w /project espressif/idf:v5.5.1 bash docker_tests/unit_tests/generate_mocks.sh`
- **Include generated headers**: Use `#include "Mockmock_header.h"` in test files
- **Configure mock expectations**: Call `functionName_ExpectAndReturn()` in test setup

## Future Enhancements

- **Code Coverage**: Integrate with gcov/lcov for coverage reports
- **Test Fixtures**: Shared setup/teardown for common scenarios
- **Parameterized Tests**: Test multiple inputs with single test function
- **Performance Testing**: Benchmark critical functions

---

**Status**: ESP32 firmware unit testing framework with advanced mocking features operational! 🧪⚡
