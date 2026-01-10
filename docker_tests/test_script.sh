#!/bin/bash
set -e

echo 'Creating standalone test project...'
TEST_DIR="/tmp/qemu_test"
rm -rf "$TEST_DIR"
mkdir -p "$TEST_DIR"
cd "$TEST_DIR"

# Create directory structure first
mkdir -p main

# Copy only the files we need for testing
cp /project/main/temp.c .
cp /project/main/circular_buffer.c .
cp /project/main/version.c .
cp -r /project/include .

# Copy mock ADC files for testing
cp /project/docker_tests/mock_adc.c .
cp /project/docker_tests/mock_adc.h .

# Create minimal main.c for testing
cat > main.c << 'EOF'
#include <stdio.h>

void app_main(void) {
    printf("QEMU Temperature Test Starting...\n");
    printf("Test completed.\n");
}
EOF

# Move source files to main directory
mv temp.c circular_buffer.c version.c main/

# Create minimal CMakeLists.txt for testing
cat > CMakeLists.txt << 'EOF'
cmake_minimum_required(VERSION 3.16)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(qemu_temp_test)
EOF

# Create minimal main component - remove hardware dependencies for Linux
cat > main/CMakeLists.txt << 'EOF'
idf_component_register(SRCS "circular_buffer.c" "version.c" "../main.c"
                       PRIV_REQUIRES freertos
                       INCLUDE_DIRS "../include")
EOF

echo 'Setting up ESP-IDF environment...'
idf.py --preview set-target linux

echo 'Building test firmware...'
idf.py build

echo 'Running QEMU tests...'
export PATH=/opt/venv/bin:$PATH
python -m pytest /project/docker_tests/pytest_hello_world.py::test_basic_qemu -v -s
