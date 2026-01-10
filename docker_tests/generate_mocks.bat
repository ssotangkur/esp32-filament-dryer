@echo off
echo Generating CMock mocks locally...

REM Build the test image if it doesn't exist
docker-compose -f "%~dp0docker-compose.yml" build

REM Run CMock in Docker using the built test image
docker run --rm -v "%~dp0..:/project" -w /project docker_tests-unit-tests bash docker_tests/unit_tests/generate_mocks.sh

echo Local mock generation process finished.
echo Check the mocks/ directory for generated files.
