@echo off
REM Change to project root directory (parent of scripts directory)
cd %~dp0..

echo Incrementing firmware patch version...
python scripts\increment_version.py
if %ERRORLEVEL% NEQ 0 (
    echo Failed to increment version
    exit /b 1
)

echo Building firmware...
REM Work around Windows CMD 8191 character command line limit.
REM Set PYTHONNOUSERSITE and use cmd /c to isolate the command execution.
set PYTHONNOUSERSITE=1
cmd /c "esp_idf_shell.bat idf.py build"
if %ERRORLEVEL% NEQ 0 (
    echo Build failed
    exit /b 1
)

echo Copying firmware binary to expected location...
if not exist build\esp32s3 mkdir build\esp32s3
copy build\esp32_filament_dryer.bin build\esp32s3\firmware.bin
if %ERRORLEVEL% NEQ 0 (
    echo Failed to copy firmware binary
    exit /b 1
)

echo Updating version.json...
python scripts\update_version.py

echo Build completed successfully!
echo Firmware is ready at build\esp32s3\firmware.bin
