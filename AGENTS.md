# ESP32 Filament Dryer Project
ESP32 project for LilyGo T-Display S3 with ESP-IDF v5.5.1 and comprehensive unit testing.

## Hardware Configuration: LilyGo T-Display S3
Target hardware specifications:
- Microcontroller: ESP32-S3R8 (Dual-core Xtensa LX7, up to 240MHz)
- Display: 1.9-inch TFT LCD (170x320 resolution, ST7789V driver)
- Memory: 8MB PSRAM, 16MB Flash
- Wireless: Wi-Fi 802.11 b/g/n, Bluetooth 5.0 LE

### Pin Configuration
Use defined pin macros from `include/product_pins.h`:
- Display: BOARD_TFT_BL (GPIO 38), BOARD_TFT_CS (GPIO 6), BOARD_TFT_DC (GPIO 7)
- I2C: SCL GPIO 17, SDA GPIO 18
- Power: BOARD_POWERON (GPIO 15)

## Build/Lint/Test Commands

### Build Commands
- Build: `esp_idf_shell.bat idf.py build`
- Clean: `esp_idf_shell.bat idf.py clean`
- Rebuild: `esp_idf_shell.bat idf.py build --rebuild-cache`

### Flash Commands
- Flash: `esp_idf_shell.bat idf.py flash`
- Monitor: `esp_idf_shell.bat idf.py monitor`
- Combined: `esp_idf_shell.bat idf.py build flash monitor`

### Test Commands
- Run all unit tests: `powershell -Command "cd docker_tests\unit_tests; .\run_unit_tests.bat"`
- Run single test file: Modify `docker_tests\unit_tests\main\CMakeLists.txt` to include only the test file you want to run
- Generate mocks: `powershell -Command "cd docker_tests\unit_tests; .\generate_mocks.bat"`
- Run specific test function: Currently requires modifying test file to run single test (Unity framework)

### Docker Test Environment
Tests run in Docker containers with CMock for automated mock generation:
- Navigate to `docker_tests\unit_tests\`
- Use `run_unit_tests.bat` for Windows execution
- Generated mocks are in `mocks/` directory
- Mock headers are defined in `mock_headers/` directory

## Code Style Guidelines

### Imports
- Always include `#include <sdkconfig.h>` first in source files
- Follow ESP-IDF include order: system includes, ESP-IDF includes, component includes, local includes
- Use angle brackets for system/ESP-IDF includes: `#include "freertos/FreeRTOS.h"`
- Use quotes for local includes: `#include "my_component.h"`
- Group includes by category with blank lines between groups

### Formatting
- Use 4-space indentation (no tabs)
- Maximum line length: 100 characters
- Use K&R brace style with opening brace on same line
- Space after keywords: `if (condition)`, `for (int i = 0; i < 10; i++)`
- No space after function names: `function_call()`
- Use consistent spacing around operators
- Use blank lines to separate logical code blocks

### Types
- Use ESP-IDF standard types: `uint8_t`, `int32_t`, etc.
- Use `bool` for boolean values (from stdbool.h)
- Use `size_t` for sizes and array indices
- Define custom types using `typedef struct` and `typedef enum`
- Use `_t` suffix for custom types: `my_struct_t`, `my_enum_t`

### Naming Conventions
- Use snake_case for functions, variables, and file names: `my_function_name()`, `my_variable`
- Use SCREAMING_SNAKE_CASE for constants and macros: `MY_CONSTANT`, `CONFIG_ENABLED`
- Use PascalCase for typedef struct names: `MyStructType_t`
- Prefix all public functions with module name: `display_init()`, `heater_set_power()`
- Use `s_` prefix for static variables: `static int s_initialized = false;`
- Use `g_` prefix for global variables: `int g_global_counter;`
- Append `_t` to typedef names: `typedef struct {} my_component_t;`

### Error Handling
- Always check ESP_ERROR_CHECK return values for ESP-IDF functions
- Use ESP-IDF error codes: `ESP_OK`, `ESP_FAIL`, `ESP_ERR_NO_MEM`, etc.
- Use descriptive error messages with ESP_LOGE
- Handle NULL pointer checks early in functions
- Use goto cleanup pattern for error handling in functions with multiple allocations
- Always check return values from malloc/free operations
- Use appropriate error logging levels: ESP_LOGE, ESP_LOGW, ESP_LOGI, ESP_LOGD

### Documentation
- Use Doxygen-style comments for all public functions
- Document all parameters, return values, and side effects
- Use `/** Brief description */` for brief function descriptions
- Use detailed comments for complex algorithms
- Include @brief, @param, @return, @note where appropriate

## ESP-IDF Environment Requirements
All ESP-IDF commands must be run through `esp_idf_shell.bat`:
- Build: `esp_idf_shell.bat idf.py build`
- Flash: `esp_idf_shell.bat idf.py flash`
- Monitor: `esp_idf_shell.bat idf.py monitor`

NEVER run `idf.py menuconfig` directly - edit `sdkconfig` files instead.

## Windows Environment Requirements
This project runs on Windows 11 with PowerShell:
- Use PowerShell syntax for all commands
- Include `cd /d "%~dp0"` at start of batch files for proper relative paths
- Use `powershell -Command` for PowerShell commands

## Git Commit Safety Requirements (CRITICAL)
NEVER commit changes automatically without explicit user approval.
Always wait for explicit "commit", "yes", "go ahead", "commit now", or "please commit" before running `git commit`.
This rule is critical and must always be followed.

When committing changes, prefer using the git commit assistant subagent:
- Use "@git_commit_assistant please commit these changes" to automatically analyze and commit changes
- The subagent will follow project conventions and create appropriate commit messages
- The subagent handles staged vs modified files appropriately

## Testing Framework
This project uses Docker-based unit testing with CMock:
- Tests located in `docker_tests/unit_tests/`
- Use CMock-generated mocks: `#include "Mockmock_component.h"`
- Follow Unity testing patterns with setUp/tearDown
- Use ignore plugins for flexible argument matching: `heap_caps_malloc_IgnoreAndReturn()`

## Skill Instructions
When working on specific tasks, consider loading these skills for detailed procedures:
- For ESP-IDF command details: `skill({ name: "esp-idf-workflow" })`
- For testing patterns: `skill({ name: "esp32-testing-patterns" })`
- For Windows development: `skill({ name: "windows-development" })`
- For subagent creation: `skill({ name: "subagent-creation" })`

## Subagents
This project includes specialized subagents for common tasks:
- For git commits: `@git_commit_assistant please commit these changes` - analyzes changes and creates appropriate commit messages following project conventions