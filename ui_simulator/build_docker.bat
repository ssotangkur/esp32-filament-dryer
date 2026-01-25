@echo off
REM Build script for UI Simulator using Docker
REM This script compiles the UI simulator using Emscripten in a Docker container
REM Can be run from project root: .\ui_simulator\build_docker.bat

REM Change to the script's directory to ensure docker-compose.yml is found
cd /d "%~dp0"

echo Building UI Simulator using Docker...

REM Check if Docker is available
docker --version >nul 2>&1
if errorlevel 1 (
    echo Error: Docker is not installed or not in PATH
    echo Please install Docker Desktop
    exit /b 1
)

REM Check if docker-compose is available
docker-compose --version >nul 2>&1
if errorlevel 1 (
    echo Error: docker-compose is not installed or not in PATH
    echo Please install docker-compose
    exit /b 1
)

echo Starting Docker build process...
echo This may take several minutes on first run as it downloads the emscripten image...

REM Run the docker-compose build
docker-compose -f docker-compose.yml up --build

if errorlevel 1 (
    echo Build failed!
    exit /b 1
)

echo Build completed successfully!
echo Open ui_simulator/build/index.html in a web browser to run the simulator
pause
