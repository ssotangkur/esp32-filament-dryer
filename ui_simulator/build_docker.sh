#!/bin/bash

# Build script for UI Simulator using Docker
# This script builds the UI simulator using pre-built LVGL libraries for fast incremental builds
# Includes detailed timing information

set -e  # Exit on any error

# Function to print timing information
print_time() {
    echo "[$(date '+%H:%M:%S')] $1"
}

START_TIME=$(date +%s)
print_time "Build started"

# Change to the script's directory
cd "$(dirname "$0")"

print_time "Starting UI Simulator build..."

# Check if Docker is available
if ! command -v docker &> /dev/null; then
    echo "Error: Docker is not installed or not in PATH"
    exit 1
fi

# Check if docker-compose is available
if ! command -v docker-compose &> /dev/null; then
    echo "Error: docker-compose is not installed or not in PATH"
    exit 1
fi

# Define directories and files to monitor
BUILD_DIR="./build"
DEPS_BUILD_DIR="./build_deps"
CACHE_DIR="./build_cache"
MANAGED_COMPONENTS="../managed_components"
MAIN_UI="../main/ui"
INCLUDE_DIR="../include"
DOCKER_ROOT="./docker_root"

# Create build directories if they don't exist
mkdir -p "$BUILD_DIR" "$DEPS_BUILD_DIR"

# Check if the build container is running
CONTAINER_CHECK_START=$(date +%s)
CONTAINER_STATUS=$(docker-compose -f docker-compose.yml ps -q ui_simulator 2>/dev/null | xargs -r docker inspect -f '{{.State.Status}}' 2>/dev/null)
CONTAINER_CHECK_END=$(date +%s)
print_time "Container status check took $((CONTAINER_CHECK_END - CONTAINER_CHECK_START))s"

# Determine if we need to rebuild dependencies
# Dependencies include LVGL and other managed components
print_time "Checking for dependency changes..."
DEPENDENCIES_CHANGED=false

# Check if managed components (LVGL) have changed since last build
if [ -f "$BUILD_DIR/.last_managed_components_check" ]; then
    if [ "$MANAGED_COMPONENTS" -nt "$BUILD_DIR/.last_managed_components_check" ]; then
        DEPENDENCIES_CHANGED=true
        print_time "Managed components have changed, dependencies need to be rebuilt"
    fi
else
    DEPENDENCIES_CHANGED=true
    print_time "First build detected, dependencies need to be built"
fi

# Check if UI source files have changed
print_time "Checking for UI source file changes..."
UI_FILES_CHANGED=false
if [ -f "$BUILD_DIR/.last_ui_check" ]; then
    # Check if any UI source files are newer than the last check
    if [ -n "$(find "$MAIN_UI" "$INCLUDE_DIR" -name '*.c' -o -name '*.h' -newer "$BUILD_DIR/.last_ui_check" 2>/dev/null)" ] || \
       [ -n "$(find "$DOCKER_ROOT" -name '*.c' -o -name '*.h' -newer "$BUILD_DIR/.last_ui_check" 2>/dev/null)" ]; then
        UI_FILES_CHANGED=true
        print_time "UI source files have changed, application needs to be rebuilt"
    fi
else
    UI_FILES_CHANGED=true
    print_time "First build detected, application needs to be built"
fi

# Check if CMake files have changed
print_time "Checking for CMake file changes..."
CMAKE_FILES_CHANGED=false
if [ -f "$BUILD_DIR/.last_cmake_check" ]; then
    if [ -n "$(find "$DOCKER_ROOT" -name 'CMakeLists.txt' -newer "$BUILD_DIR/.last_cmake_check" 2>/dev/null)" ]; then
        CMAKE_FILES_CHANGED=true
        print_time "CMake files have changed, configuration needs to be updated"
    fi
else
    CMAKE_FILES_CHANGED=true
    print_time "First build detected, CMake configuration needs to be generated"
fi

# Determine build strategy
if [ "$DEPENDENCIES_CHANGED" = true ]; then
    print_time "Dependencies have changed - performing full rebuild"
    BUILD_TYPE="full"
elif [ "$CMAKE_FILES_CHANGED" = true ]; then
    print_time "CMake files have changed - regenerating configuration"
    BUILD_TYPE="configure_only"
elif [ "$UI_FILES_CHANGED" = true ]; then
    print_time "UI files have changed - performing incremental build"
    BUILD_TYPE="incremental"
else
    print_time "No changes detected - build is up to date"
    touch "$BUILD_DIR/.last_build_check"
    END_TIME=$(date +%s)
print_time "Build completed successfully in $((END_TIME - START_TIME))s!"
    exit 0
fi

# Start the container if it's not running
if [ -z "$CONTAINER_STATUS" ] || [ "$CONTAINER_STATUS" != "running" ]; then
    print_time "Starting build container..."
    START_CONTAINER_TIME=$(date +%s)
    docker-compose -f docker-compose.yml up -d --build ui_simulator
    # Wait a moment for the container to be ready
    sleep 5
    END_CONTAINER_TIME=$(date +%s)
    print_time "Container startup took $((END_CONTAINER_TIME - START_CONTAINER_TIME))s"
fi

# Execute the build inside the running container based on build type
print_time "Executing build inside container (type: $BUILD_TYPE)..."
BUILD_START=$(date +%s)

if [ "$BUILD_TYPE" = "full" ] || [ "$BUILD_TYPE" = "configure_only" ]; then
    # For full or configure-only builds, we need to rebuild dependencies and app
    docker-compose -f docker-compose.yml exec -T ui_simulator bash -c "
      cd /emsdk && 
      source ./emsdk_env.sh && 
      cd /project/ui_simulator &&
      mkdir -p /project/build &&
      emcmake cmake . -B /project/build --log-level=NOTICE &&
      emmake make -C /project/build -j\$(nproc --all)
    "
else
    # For incremental builds, we only rebuild the application part
    docker-compose -f docker-compose.yml exec -T ui_simulator bash -c "
      cd /emsdk && 
      source ./emsdk_env.sh && 
      cd /project/ui_simulator &&
      mkdir -p /project/build &&
      emcmake cmake . -B /project/build --log-level=NOTICE &&
      emmake make -C /project/build index -j\$(nproc --all)
    "
fi

BUILD_END=$(date +%s)
print_time "Build execution inside container took $((BUILD_END - BUILD_START))s"

# Update timestamps to reflect the last successful build/check
touch "$BUILD_DIR/.last_managed_components_check"
touch "$BUILD_DIR/.last_ui_check" 
touch "$BUILD_DIR/.last_cmake_check"
touch "$BUILD_DIR/.last_build_check"

END_TIME=$(date +%s)
print_time "Incremental build completed successfully in $((END_TIME - START_TIME))s!"
echo "Open ui_simulator/build/index.html in a web browser to run the simulator"