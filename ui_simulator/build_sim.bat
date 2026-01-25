@echo off
REM Build script for UI Simulator
REM This script compiles the UI simulator using Emscripten
REM Can be run from project root: .\ui_simulator\build_sim.bat

REM Change to the script's directory to ensure paths are correct
cd /d "%~dp0"

echo Building UI Simulator...

REM Check if emcc is available
where emcc >nul 2>nul
if errorlevel 1 (
    echo Error: Emscripten (emcc) is not installed or not in PATH
    echo Please install Emscripten SDK and activate it
    exit /b 1
)

REM Create build directory if it doesn't exist
if not exist build mkdir build

echo Compiling with Emscripten...

REM Compile the simulator with Emscripten
emcc src/main.c ^
    -I ../managed_components/lvgl__lvgl ^
    -I ../managed_components/lvgl__lvgl/src ^
    -I ../managed_components/lvgl__lvgl/examples ^
    -I ../managed_components/lvgl__lvgl/demos ^
    -I src ^
    -llvgl ^
    -D BUILD_FOR_SIMULATOR ^
    -s USE_SDL=2 ^
    -s ALLOW_MEMORY_GROWTH=1 ^
    -s ASYNCIFY ^
    -s MODULARIZE ^
    -s EXPORT_NAME='Module' ^
    -s ENVIRONMENT=web,shell ^
    -s NO_EXIT_RUNTIME=1 ^
    -s SUPPORT_LONGJMP=0 ^
    -s WASM=1 ^
    --shell-file shell.html ^
    -o build/index.html ^
    -O2

if errorlevel 1 (
    echo Build failed!
    exit /b 1
)

echo Build completed successfully!
echo Open ui_simulator/build/index.html in a web browser to run the simulator
pause
