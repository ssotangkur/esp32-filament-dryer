@echo off
echo Running ESP32 Unit Tests in Docker...

REM Build and run the unit tests in Docker
docker-compose build
docker-compose run --rm unit-tests

echo Unit tests completed!
