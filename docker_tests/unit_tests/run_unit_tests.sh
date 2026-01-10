#!/bin/bash
set -e

echo "Building and running ESP32 Unit Tests with CMock..."

# Clean any existing build directory
echo "Cleaning build directory..."
rm -rf build

# Generate CMock mocks
echo "Generating CMock mocks..."
cd main
ruby /opt/cmock/lib/cmock.rb --mock_prefix=Mock --mock_path=. --unity_path=/opt/unity esp_adc_mock_headers.h
cd ..

# Build the unit tests using plain CMake
echo "Building unit tests with CMake..."
mkdir -p build
cd build
cmake .. -G Ninja
ninja

echo "Running unit tests..."
# Run the unit tests
if [ -f "./unit_tests" ]; then
    ./unit_tests
    echo "Unit tests completed!"
else
    echo "Unit test executable not found!"
    exit 1
fi
