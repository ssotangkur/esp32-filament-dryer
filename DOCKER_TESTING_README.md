# ESP32 Firmware Unit Testing with CMock

This guide explains how to run automated unit tests for your ESP32 filament dryer firmware using the Unity testing framework with CMock for automatic mock generation.

## Overview

Your project includes a comprehensive unit testing framework that allows testing individual components in isolation without hardware dependencies.

## Testing Framework Structure

```
docker_tests/unit_tests/
├── CMakeLists.txt                    # Main test build configuration
├── run_unit_tests.sh               # Test runner script
├── test_framework.h               # Common test utilities
├── circular_buffer_simple.c       # Simple buffer implementation for testing
├── circular_buffer_simple.h       # Simple buffer header
├── test_circular_buffer_simple.c  # Circular buffer unit tests
├── test_main.c                    # Main test runner
├── main/
│   ├── CMakeLists.txt             # Test component configuration
│   ├── test_circular_buffer.c     # Circular buffer tests
│   ├── test_main.c               # Main function tests
│   ├── test_temperature.c        # Temperature calculation tests
│   └── test_version.c            # Version management tests
```

## Running Unit Tests

### Prerequisites
- Docker Desktop installed and running
- Windows 10/11 with WSL2 (recommended) or Hyper-V

### Quick Start
```batch
# Navigate to docker_tests directory
cd docker_tests

# Run all unit tests in Docker
run_unit_tests.bat
```

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

## Test Categories

### ✅ **Circular Buffer Tests**
- **Location**: `test_circular_buffer.c`, `test_circular_buffer_simple.c`
- **Coverage**: Buffer initialization, data insertion/removal, overflow handling
- **Mock Dependencies**: None (pure algorithm testing)

### ✅ **Version Management Tests**
- **Location**: `test_version.c`
- **Coverage**: Version string parsing, increment logic, format validation
- **Mock Dependencies**: File I/O operations

### ✅ **Temperature Calculation Tests**
- **Location**: `test_temperature.c`
- **Coverage**: Steinhart-Hart equation implementation, ADC value conversion
- **Mock Dependencies**: ADC hardware interface

### ✅ **Main Function Tests**
- **Location**: `test_main.c`
- **Coverage**: Application initialization, component startup sequence
- **Mock Dependencies**: ESP-IDF system calls, hardware peripherals

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

## Benefits of Unit Testing

✅ **Fast Execution**: Tests run in seconds, not minutes
✅ **Isolated Testing**: Each component tested independently
✅ **No Hardware Required**: Perfect for CI/CD pipelines
✅ **Early Bug Detection**: Catch issues during development
✅ **Regression Prevention**: Ensure changes don't break existing functionality
✅ **Documentation**: Tests serve as usage examples

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
- Generate mocks for new dependencies: `cmock --mock_prefix=Mock <header.h>`
- Include mock headers in test files
- Configure mock expectations properly

## Future Enhancements

- **Code Coverage**: Integrate with gcov/lcov for coverage reports
- **Test Fixtures**: Shared setup/teardown for common scenarios
- **Parameterized Tests**: Test multiple inputs with single test function
- **Performance Testing**: Benchmark critical functions

---

**Status**: ESP32 firmware unit testing framework ready! 🧪⚡
