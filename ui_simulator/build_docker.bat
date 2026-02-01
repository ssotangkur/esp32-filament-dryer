@echo off
REM Build script for UI Simulator using Docker
REM This script builds the UI simulator using pre-built LVGL libraries for fast incremental builds
REM Includes detailed timing information

setlocal enabledelayedexpansion

REM Change to script directory
pushd "%~dp0"

REM Get start time for timing
for /f "tokens=1-4 delims=:.," %%a in ("%time%") do set /a "start=(((%%a*60)+1%%b %% 100)*60+1%%c %% 100)*100+1%%d %% 100"

echo [%time%] Build started
echo [%time%] Starting UI Simulator build...

REM Check if Docker is available
docker --version >nul 2>&1
if errorlevel 1 (
    echo Error: Docker is not installed or not in PATH
    echo Please install Docker Desktop
    popd
    exit /b 1
)

REM Check if docker-compose is available
docker-compose --version >nul 2>&1
if errorlevel 1 (
    echo Error: docker-compose is not installed or not in PATH
    echo Please install docker-compose
    popd
    exit /b 1
)

REM Define directories and files to monitor
set BUILD_DIR=./build

REM Create build directory if it doesn't exist
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

REM Check if the build container is running
for /f "tokens=*" %%i in ('docker-compose -f docker-compose.yml ps -q ui_simulator ^| findstr .') do set CONTAINER_ID=%%i

REM CMake handles incremental builds natively - just run the build
set BUILD_TYPE=incremental

REM Start the container if it's not running
if not defined CONTAINER_ID (
    echo [%time%] Starting build container...
    set START_CONTAINER=%time%
    docker-compose -f docker-compose.yml up -d --build ui_simulator
    REM Wait a moment for the container to be ready
    ping -n 6 127.0.0.1 >nul
    set END_CONTAINER=%time%
    for /f "tokens=1-4 delims=:.," %%a in ("!START_CONTAINER!") do set /a "start_cont=(((%%a*60)+1%%b %% 100)*60+1%%c %% 100)*100+1%%d %% 100"
    for /f "tokens=1-4 delims=:.," %%a in ("!END_CONTAINER!") do set /a "end_cont=(((%%a*60)+1%%b %% 100)*60+1%%c %% 100)*100+1%%d %% 100"
    set /a "cont_duration=end_cont-start_cont"
    echo [%time%] Container startup took !cont_duration!cs
) else (
    REM Check if container is running
    for /f "tokens=*" %%i in ('docker inspect -f "{{.State.Running}}" %CONTAINER_ID% 2^>nul') do set IS_RUNNING=%%i
    if "!IS_RUNNING!" neq "true" (
        echo [%time%] Starting build container...
        set START_CONTAINER=%time%
        docker-compose -f docker-compose.yml up -d --build ui_simulator
        ping -n 6 127.0.0.1 >nul
        set END_CONTAINER=%time%
        for /f "tokens=1-4 delims=:.," %%a in ("!START_CONTAINER!") do set /a "start_cont=(((%%a*60)+1%%b %% 100)*60+1%%c %% 100)*100+1%%d %% 100"
        for /f "tokens=1-4 delims=:.," %%a in ("!END_CONTAINER!") do set /a "end_cont=(((%%a*60)+1%%b %% 100)*60+1%%c %% 100)*100+1%%d %% 100"
        set /a "cont_duration=end_cont-start_cont"
        echo [%time%] Container startup took !cont_duration!cs
    )
)

REM Execute the build inside the running container
echo [%time%] Executing build inside container...
set BUILD_START=%time%

REM Run cmake and make - CMake handles incremental builds automatically
REM Pre-built libraries include demos and examples for maximum flexibility
docker-compose -f docker-compose.yml exec -T ui_simulator bash -c "cd /emsdk && source ./emsdk_env.sh && cd /project/ui_simulator && mkdir -p /project/build && emcmake cmake . -B /project/build --log-level=NOTICE && emmake make -C /project/build -j$(nproc --all)"

set BUILD_END=%time%
for /f "tokens=1-4 delims=:.," %%a in ("!BUILD_START!") do set /a "start_build=(((%%a*60)+1%%b %% 100)*60+1%%c %% 100)*100+1%%d %% 100"
for /f "tokens=1-4 delims=:.," %%a in ("!BUILD_END!") do set /a "end_build=(((%%a*60)+1%%b %% 100)*60+1%%c %% 100)*100+1%%d %% 100"
set /a "build_duration=end_build-start_build"
echo [%time%] Build execution inside container took !build_duration!cs

REM Mark build as complete
echo [%time%] Build completed successfully

goto :end_with_timing

:end_with_timing
REM Calculate total time
for /f "tokens=1-4 delims=:.," %%a in ("%time%") do set /a "end=(((%%a*60)+1%%b %% 100)*60+1%%c %% 100)*100+1%%d %% 100"
set /a "total=end-start"
echo [%time%] Build completed successfully in !total!cs!
echo Open ui_simulator/build/index.html in a web browser to run the simulator

REM Restore original directory
popd

:end