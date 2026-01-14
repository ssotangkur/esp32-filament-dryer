# Gemini Project Rules

This document outlines the rules and guidelines for the Gemini CLI agent within this project. These rules are sourced from the `.clinerules` directory.

## ESP-IDF Rules

# ESP-IDF Workspace Rules

## ESP-IDF Command Usage

ESP-IDF commands like `idf.py`, `esptool.py`, etc. must be run through the `esp_idf_shell.bat` script to properly set up the environment.

### Correct Usage:
```bash
esp_idf_shell.bat idf.py build
esp_idf_shell.bat idf.py flash
esp_idf_shell.bat idf.py monitor
```

### Why this is necessary:
1. The `esp_idf_shell.bat` script sets up required environment variables
2. It configures the correct Python environment
3. It establishes proper PATH settings for ESP-IDF tools
4. It creates DOSKEY macros for ESP-IDF commands

### Common Commands:
- Build: `esp_idf_shell.bat idf.py build`
- Flash: `esp_idf_shell.bat idf.py flash`
- Monitor: `esp_idf_shell.bat idf.py monitor`
- Full build & flash: `esp_idf_shell.bat idf.py build flash monitor`

### Error Prevention:
Running ESP-IDF commands directly (e.g., `idf.py build`) will result in:
```
'idf.py' is not recognized as an internal or external command
```

Always use `esp_idf_shell.bat` as a prefix to ESP-IDF commands.

## Interactive Commands to Avoid

### Menuconfig
Do not run `idf.py menuconfig` or `esp_idf_shell.bat idf.py menuconfig` as these commands are interactive and require user input through a terminal-based menu interface.

### Why avoid menuconfig:
1. The menuconfig interface requires interactive navigation with arrow keys, enter, and other inputs
2. As an AI assistant, I cannot interact with or navigate the menuconfig interface
3. Running menuconfig will cause the command to hang waiting for user input
4. Configuration changes should be made by directly editing `sdkconfig` or `sdkconfig.defaults` files

### Alternative approaches:
- Edit `sdkconfig` or `sdkconfig.defaults` directly with desired CONFIG_xxx=y settings
- Use `idf.py save-defconfig` to save current configuration to defaults
- Use `idf.py reconfigure` to apply configuration changes


## Git Rules

# Git Commit Rules

## Commit Behavior

NEVER commit changes automatically. Always wait for explicit user approval before running `git commit`.

### Prohibited Actions:
- Do not run `git commit` without explicit user instruction
- Do not assume permission to commit based on successful builds
- Do not commit "working solutions" or "completed tasks" automatically

### Required Actions:
- Always ask user permission before committing
- Wait for explicit "yes", "commit", or "go ahead" before running commit commands
- If user says "don't commit automatically anymore", strictly adhere to this rule

### Exception:
Only commit immediately if user explicitly says something like:
- "Commit this now"
- "Go ahead and commit"
- "Commit the changes"
- "Please commit"

### Reminder:
This rule prevents accidental commits and ensures user has full control over when changes are committed to version control.


## Hardware Rules

# Hardware Configuration Rules

## Target Hardware: LilyGo T-Display S3

This project is developed for the LilyGo T-Display S3 development board.

### Key Specifications:
- **Microcontroller**: ESP32-S3R8 (Dual-core Xtensa LX7, up to 240MHz)
- **Display**: 1.9-inch TFT LCD (170x320 resolution, ST7789V driver)
- **Touch**: No touchscreen
- **Battery**: LiPo battery connector with charging circuit (supports up to 3.7V/1000mAh)
- **USB**: USB-C for programming and power supply
- **Buttons**: Power button, Reset button
- **LED**: RGB LED indicator
- **Sensors**: Optional BMA423 3-axis accelerometer (I2C interface)
- **Memory**: 8MB PSRAM, 16MB Flash
- **Wireless**: Wi-Fi 802.11 b/g/n, Bluetooth 5.0 LE

### Pin Configuration (from product_pins.h):
- **Power**: GPIO 15 (BOARD_POWERON)
- **Display**:
  - Backlight: GPIO 38 (BOARD_TFT_BL)
  - Data bus: GPIO 39-48 (BOARD_TFT_DATA0-7)
  - Reset: GPIO 5 (BOARD_TFT_RST)
  - Chip Select: GPIO 6 (BOARD_TFT_CS)
  - Data/Command: GPIO 7 (BOARD_TFT_DC)
  - Write: GPIO 8 (BOARD_TFT_WR)
  - Read: GPIO 9 (BOARD_TFT_RD)
- **I2C**: SCL GPIO 17, SDA GPIO 18
- **Touch**:
  - Interrupt: GPIO 16 (BOARD_TOUCH_IRQ)
  - Reset: GPIO 21 (BOARD_TOUCH_RST)

### Display Configuration:
- Resolution: 170x320 pixels
- Driver: ST7789V
- Interface: 8-bit parallel
- Buffer size: 170x100 pixels (partial buffering)

### Important Notes:
- This board variant does not include touchscreen functionality
- Uses partial display buffering to conserve memory
- ESP32-S3 supports high-speed interfaces suitable for the parallel display bus
- Ensure proper power management when using battery operation

### Development Considerations:
- Use the defined pin macros from `include/product_pins.h` for hardware access
- The display component `esp_lcd_st7735` is configured for ST7789V compatibility
- Consider memory constraints when designing graphics-heavy applications
- Touch-related pins (BOARD_TOUCH_IRQ, BOARD_TOUCH_RST) are not connected on this variant


## Testing Rules

# ESP32 Unit Testing Rules

## Overview
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
#include "Mockmock_component.h"      # CMock-generated mocks
#include "../../main/source_file.c"  # Source under test

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
docker run --rm -v ${pwd}:/project espressif/idf:v5.5.1 \
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


## Windows Rules

# Windows Environment Rules

## Operating System: Windows 11

This project is developed on Windows 11 with Visual Studio Code as the primary IDE.

## Command Execution

All CLI commands must be executed using PowerShell syntax and commands.

### Correct Usage:
- Use PowerShell cmdlets and syntax for all command-line operations
- Avoid bash or Unix-style commands unless explicitly compatible
- When executing commands, ensure they are formatted for PowerShell

### Why this is necessary:
1. The default shell environment is cmd.exe, but PowerShell provides better scripting capabilities
2. PowerShell offers consistent command syntax across different Windows systems
3. It supports more advanced automation and error handling features
4. Ensures compatibility with Windows-specific tools and environments

### Common PowerShell Commands:
- Get-ChildItem (equivalent to ls/dir)
- Set-Location (equivalent to cd)
- New-Item (equivalent to touch/mkdir)
- Remove-Item (equivalent to rm/del)

### Error Prevention:
Running bash-style commands (e.g., `ls`, `cd /path`) directly may fail with errors like:
```
'ls' is not recognized as an internal or external command
```

Always use PowerShell equivalents and syntax for command execution.
