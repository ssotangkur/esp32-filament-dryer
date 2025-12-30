@echo off
echo Incrementing firmware patch version...
python scripts\increment_version.py
if %ERRORLEVEL% NEQ 0 (
    echo Failed to increment version
    exit /b 1
)

echo Building firmware...
echo idf.py build > temp_build.cmd
call esp_idf_shell.bat temp_build.cmd
del temp_build.cmd
if %ERRORLEVEL% NEQ 0 (
    echo Build failed
    exit /b 1
)

echo Copying firmware binary to expected location...
copy build\esp32_filament_dryer.bin build\esp32s3\firmware.bin
if %ERRORLEVEL% NEQ 0 (
    echo Failed to copy firmware binary
    exit /b 1
)

echo Updating version.json...
python scripts\update_version.py

echo Build completed successfully!
echo Firmware is ready at build\esp32s3\firmware.bin
