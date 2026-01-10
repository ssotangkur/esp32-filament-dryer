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
)

# Automatically find all generated mock .c files
file(GLOB MOCK_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/mocks/*.c")

# Create a simplified circular buffer for testing (no FreeRTOS)
set(SOURCES
    main/test_main.c
    main/test_circular_buffer_simple.c
    main/test_version.c
    main/test_temperature.c
    ${MOCK_SOURCES}                   # All CMock-generated mocks
    /opt/unity/src/unity.c            # Unity testing framework
    /opt/cmock/src/cmock.c            # CMock framework
    /project/main/version.c           # Version source from main project
    # Note: circular_buffer_simple.c is included by main/test_circular_buffer_simple.c
)

# Include directories for Unity and CMock
include_directories(
    /opt/unity/src      # Unity framework headers
    /opt/cmock/src      # CMock headers
)

# Create executable
add_executable(unit_tests ${SOURCES})

# Link math library
target_link_libraries(unit_tests m)
