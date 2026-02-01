#!/bin/bash

# Build script for UI Simulator inside Docker container
# Supports both full and incremental builds with timing

set -e  # Exit on any error

# Source the Emscripten environment
cd /emsdk
source ./emsdk_env.sh
cd /project/ui_simulator

echo "Setting up build environment..."

# Create build directory
BUILD_DIR="/project/build"
mkdir -p "$BUILD_DIR"

# Function to print timing information
print_time() {
    echo "[$(date '+%H:%M:%S')] $1"
}

START_TIME=$(date +%s)
print_time "Build started"

# Check if we have a previous build configuration
if [ -f "$BUILD_DIR/build.ninja" ] || [ -f "$BUILD_DIR/Makefile" ]; then
    print_time "Previous build configuration found, checking for changes..."

    # Run CMake to regenerate build files if needed (this handles incremental configuration)
    print_time "Updating CMake configuration if needed..."
    CMAKE_START=$(date +%s)
    emcmake cmake . -B "$BUILD_DIR" --log-level=NOTICE -DCONFIG_LV_BUILD_DEMOS=OFF -DCONFIG_LV_BUILD_EXAMPLES=OFF
    CMAKE_END=$(date +%s)
    CMAKE_DURATION=$((CMAKE_END - CMAKE_START))
    print_time "CMake configuration took ${CMAKE_DURATION}s"

    # Perform incremental build
    print_time "Performing incremental build..."
    MAKE_START=$(date +%s)
    emmake make -C "$BUILD_DIR" -j$(nproc)
    MAKE_END=$(date +%s)
    MAKE_DURATION=$((MAKE_END - MAKE_START))
    print_time "Make build took ${MAKE_DURATION}s"

else
    print_time "No previous build configuration found, performing full build..."

    # Generate build files and build
    print_time "Generating build configuration..."
    CMAKE_START=$(date +%s)
    emcmake cmake . -B "$BUILD_DIR" --log-level=NOTICE -DCONFIG_LV_BUILD_DEMOS=OFF -DCONFIG_LV_BUILD_EXAMPLES=OFF
    CMAKE_END=$(date +%s)
    CMAKE_DURATION=$((CMAKE_END - CMAKE_START))
    print_time "CMake configuration took ${CMAKE_DURATION}s"

    print_time "Performing full build..."
    MAKE_START=$(date +%s)
    emmake make -C "$BUILD_DIR" -j$(nproc)
    MAKE_END=$(date +%s)
    MAKE_DURATION=$((MAKE_END - MAKE_START))
    print_time "Make build took ${MAKE_DURATION}s"
fi

END_TIME=$(date +%s)
TOTAL_DURATION=$((END_TIME - START_TIME))

print_time "Build completed successfully in ${TOTAL_DURATION}s!"
echo "Find the output in ui_simulator/build/"

# Keep container running to allow for multiple incremental builds
exec tail -f /dev/null