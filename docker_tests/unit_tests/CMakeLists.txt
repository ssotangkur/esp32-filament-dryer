# Simple unit test project for basic functionality
cmake_minimum_required(VERSION 3.5)
project(unit_tests)

# Set C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Include directories
include_directories(
    /project/include      # Project include directory
    /project/main         # Project main directory
    ${CMAKE_CURRENT_SOURCE_DIR}/main     # Test main directory
    ${CMAKE_CURRENT_SOURCE_DIR}/mocks    # Generated mock headers
    ${CMAKE_CURRENT_SOURCE_DIR}/mock_headers  # Mock header definitions
    ${CMAKE_CURRENT_SOURCE_DIR}/mock_headers/freertos  # FreeRTOS mock headers
)

# Automatically find all generated mock .c files
file(GLOB MOCK_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/mocks/*.c")

# Unit test sources for ESP32 circular buffer testing
set(SOURCES
    main/test_main.c
    main/test_circular_buffer.c       # Real circular buffer tests with ESP-IDF/FreeRTOS
    main/test_temp.c                  # Temperature sensor tests
    main/test_version.c
    main/test_temperature.c
    ${MOCK_SOURCES}                   # All CMock-generated mocks
    /opt/unity/src/unity.c            # Unity testing framework
    /opt/cmock/src/cmock.c            # CMock framework
    ${CMAKE_CURRENT_BINARY_DIR}/cmock_globals.c  # Global variables for CMock plugins
    /project/main/version.c           # Version source from main project
    /project/main/temp.c              # Temperature sensor source for testing
    # Note: circular_buffer.c is included by main/test_circular_buffer.c
)

# Include directories for Unity and CMock
include_directories(
    /opt/unity/src      # Unity framework headers
    /opt/cmock/src      # CMock headers
)

# Define global variables required by CMock ignore plugins
# These must be defined once and shared across all mocks
# Create a globals file
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/cmock_globals.c
"/* Global variables required by CMock ignore plugins */
int GlobalExpectCount = 0;
int GlobalVerifyOrder = 0;
")

# Create executable
add_executable(unit_tests ${SOURCES})

# Link math library
target_link_libraries(unit_tests m)
