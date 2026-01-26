@echo off
echo Starting local server for UI Simulator...
echo Open your browser and go to: http://localhost:8000
echo Press Ctrl+C to stop the server
cd /d "%~dp0\build"
python -m http.server 8000