#!/bin/bash

# Build script for UI Simulator using Docker
# This script compiles the UI simulator using Emscripten in a Docker container
# Can be run from project root: ./ui_simulator/build_docker.sh

# Change to the script's directory to ensure docker-compose.yml is found
cd "$(dirname "$0")"

echo "Building UI Simulator using Docker..."

# Check if Docker is available
if ! command -v docker &> /dev/null; then
    echo "Error: Docker is not installed or not in PATH"
    echo "Please install Docker Desktop or Docker Engine"
    exit 1
fi

# Check if docker-compose is available
if ! command -v docker-compose &> /dev/null; then
    echo "Error: docker-compose is not installed or not in PATH"
    echo "Please install docker-compose"
    exit 1
fi

echo "Starting Docker build process..."
echo "This may take several minutes on first run as it downloads the emscripten image..."

# Run the docker-compose build
docker-compose -f docker-compose.yml up --build

echo "Build completed successfully!"
echo "Open ui_simulator/build/index.html in a web browser to run the simulator"
