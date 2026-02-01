#!/bin/bash
# Pre-build script for LVGL and SDL libraries
# This runs during Docker image build to create cached libraries

set -e

echo "Pre-building LVGL and SDL libraries..."

# Source the Emscripten environment
cd /emsdk
source ./emsdk_env.sh

# Create build directory for pre-built libraries
mkdir -p /tmp/lvgl_prebuild
cd /tmp/lvgl_prebuild

# Copy necessary files for the build
mkdir -p managed_components
cp -r /project/managed_components/lvgl__lvgl managed_components/

# Copy the prebuild CMakeLists.txt
cp /project/ui_simulator/docker_root/CMakeLists_prebuilt.txt CMakeLists.txt

# Copy lv_conf.h to the build directory root (where CMake expects it)
cp /project/ui_simulator/docker_root/lv_conf.h .

# Build the libraries
echo "Configuring pre-build..."
# Include demos and examples for maximum flexibility
emcmake cmake . -B build --log-level=NOTICE \
    -DCONFIG_LV_BUILD_DEMOS=ON \
    -DCONFIG_LV_BUILD_EXAMPLES=ON \
    -DCMAKE_INSTALL_PREFIX=/usr/local

echo "Building libraries..."
emmake make -C build -j$(nproc --all)

echo "Installing libraries..."
emmake make -C build install

# Clean up build files but keep the installed libraries
cd /
rm -rf /tmp/lvgl_prebuild

echo "Pre-built libraries installed to /usr/local/lib"
echo "Library files:"
ls -la /usr/local/lib/
