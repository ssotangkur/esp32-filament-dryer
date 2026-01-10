@echo off
echo 🚀 Running ESP32 Firmware Tests with QEMU in Docker
echo ====================================================

REM Change to the docker_tests directory
cd /d "%~dp0"

REM Build test image
echo 🏗️  Building test Docker image...
docker-compose build

REM Run QEMU tests
echo 🧪 Running QEMU tests...
docker-compose run --rm idf-test

REM Run unit tests
echo 🧪 Running unit tests...
docker-compose run --rm idf-test bash -c "cd docker_tests/unit_tests && ./run_unit_tests.sh"

echo ✅ All tests completed!
