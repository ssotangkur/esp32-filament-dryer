# Simple unit test project for basic functionality
cmake_minimum_required(VERSION 3.5)
project(unit_tests)

# Set C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../main
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/main
)

# Create a simplified circular buffer for testing (no FreeRTOS)
set(SOURCES
    test_main.c
    test_circular_buffer_simple.c
    main/test_version.c
    main/test_temperature.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../main/version.c
    # Note: circular_buffer_simple.c is included by test_circular_buffer_simple.c
)

# Create executable
add_executable(unit_tests ${SOURCES})

# Link math library
target_link_libraries(unit_tests m)
