@echo off
echo Incrementing firmware patch version...
python scripts\increment_version.py
if %ERRORLEVEL% NEQ 0 (
    echo Failed to increment version
    exit /b 1
)

echo Building firmware...
esp_idf_shell.bat idf.py build
if %ERRORLEVEL% NEQ 0 (
    echo Build failed
    exit /b 1
)

echo Updating version.json...
python scripts\update_version.py

echo Build completed successfully!
echo Firmware is ready at build\esp32s3\firmware.bin
