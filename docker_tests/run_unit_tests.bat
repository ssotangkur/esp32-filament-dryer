@echo off
echo Running ESP32 Unit Tests in Docker...

REM Change to the script's directory to ensure docker-compose.yml is found
cd /d "%~dp0"

REM Build and run the unit tests in Docker
docker-compose build
if %ERRORLEVEL% neq 0 (
    echo Build failed!
    exit /b 1
)

docker-compose run --rm unit-tests
if %ERRORLEVEL% neq 0 (
    echo Unit tests failed!
    exit /b 1
)

echo Unit tests completed!
