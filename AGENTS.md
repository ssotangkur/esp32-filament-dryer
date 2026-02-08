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

## Subagents & Skills Catalog

### @idf subagent (CRITICAL)
ESP-IDF build, flash, monitor, clean, fullclean, and reconfigure operations.
**Usage:** `@idf build`, `@idf flash`, `@idf monitor`, `@idf clean`, `@idf fullclean`, `@idf reconfigure`
**Required for:** All ESP-IDF command execution.

### @git_commit_assistant subagent (CRITICAL)
Analyzes changes and creates appropriate commit messages following project conventions.
**Usage:** `@git_commit_assistant please commit these changes`
**Required for:** All git commits (see p.X).

### @ui_simulator subagent
Builds the UI simulator using Docker and Emscripten for WebAssembly testing.
**Usage:** `@ui_simulator build`
**Required for:** Building and testing the LVGL UI in a web browser.
**Optional:** Add a visual processing request to automatically analyze the UI output using the url_snapshot agent.
**Example:** `@ui_simulator build and tell me what temperature is displayed`

### @url_snapshot subagent
Navigates to a URL using Chrome DevTools MCP, takes a snapshot/screenshot, and performs visual analysis.
**Usage:** `@url_snapshot navigate to <url> and <visual processing request>`
**Required for:** Visual testing and analysis of UI simulator or web pages.
**Example:** `@url_snapshot navigate to file:///D:/Projects/esp32/esp32_filament_dryer/ui_simulator/build/index.html and tell me what temperature is displayed`

### Skill Instructions
Load specialized skills for detailed procedures when working on complex tasks:

| Skill | When to Load |
|-------|-------------|
| `skill({ name: "esp-idf-workflow" })` | Building, flashing, or debugging ESP-IDF projects; needing detailed command syntax |
| `skill({ name: "esp32-testing-patterns" })` | Writing or debugging unit tests; generating mocks; working with CMock/Unity |
| `skill({ name: "windows-development" })` | Creating or modifying batch/PowerShell scripts; path handling issues |
| `skill({ name: "subagent-creation" })` | Creating new subagents or understanding subagent configuration |

**Tip:** Skills provide detailed step-by-step guidance and best practices beyond what's in this file. Load them when you need comprehensive workflows.

## Build/Lint/Test Commands

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

## ESP-IDF Subagent Requirement (CRITICAL)
All ESP-IDF build, flash, monitor, clean, and reconfigure commands MUST be executed through the `@idf` subagent.
**See:** Subagents & Skills Catalog (p.X) for available commands and usage.

## ESP-IDF Environment Requirements
This project uses ESP-IDF v5.5.1 on Windows 11.

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

When committing changes, you MUST use the git commit assistant subagent (see Subagents & Skills Catalog, p.X).
Manual commits using `git commit` directly are prohibited except in emergency situations.

## Testing Framework
This project uses Docker-based unit testing with CMock:
- Tests located in `docker_tests/unit_tests/`
- Use CMock-generated mocks: `#include "Mockmock_component.h"`
- Follow Unity testing patterns with setUp/tearDown
- Use ignore plugins for flexible argument matching: `heap_caps_malloc_IgnoreAndReturn()`