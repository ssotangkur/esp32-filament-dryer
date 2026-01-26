@echo off
echo Building UI Simulator with Docker...
echo This may take several minutes. Please be patient.
docker-compose down 2>nul
docker-compose up --build --abort-on-container-exit
if %ERRORLEVEL% EQU 0 (
    echo.
    echo Build completed successfully!
    echo Output files are in the ui_simulator\build directory
) else (
    echo.
    echo Build failed. Check the error messages above.
)