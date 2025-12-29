@echo off

:: ESP-IDF Shell Setup Batch File
:: This script sets up the ESP-IDF environment and executes commands passed as arguments

:: Set IDF_TOOLS_PATH if not already set
if "%IDF_TOOLS_PATH%" == "" (
    set IDF_TOOLS_PATH=D:\Espressif
    echo IDF_TOOLS_PATH not set. Setting to D:\Espressif
)

:: Add IDF_TOOLS_PATH to PATH
set PATH=%IDF_TOOLS_PATH%;%PATH%

:: Set temporary files for IDF configuration
set TEMP_IDF_PYTHON_PATH="%TEMP%\idf-python-path.txt"
set TEMP_IDF_PATH=%TEMP%\idf-path.txt

:: Use the ESP-IDF ID provided in the original command
set PARAM=esp-idf-f27b302856cbf3a7065694c7cc7f9de5
idf-env config get --property "path" --idf-id %PARAM%>%TEMP_IDF_PATH%
idf-env config get --property python --idf-id %PARAM%>%TEMP_IDF_PYTHON_PATH%

:: Read the configuration
set /P IDF_PATH=<"%TEMP_IDF_PATH%"
set /P IDF_PYTHON=<%TEMP_IDF_PYTHON_PATH%

:: Get Git path
set TEMP_IDF_GIT_PATH="%TEMP%\idf-git-path.txt"
idf-env config get --property gitPath>%TEMP_IDF_GIT_PATH%
set /P IDF_GIT=<%TEMP_IDF_GIT_PATH%

:: Set up Python and Git paths
for %%F in (%IDF_PYTHON%) do set IDF_PYTHON_DIR=%%~dpF
for %%F in (%IDF_GIT%) do set IDF_GIT_DIR=%%~dpF

:: Add Python and Git paths to PATH
set "PATH=%IDF_PYTHON_DIR%;%IDF_GIT_DIR%;%PATH%"

:: Clear environment variables that might interfere
if not "%PYTHONPATH%"=="" (
    echo Clearing PYTHONPATH, was set to %PYTHONPATH%
    set PYTHONPATH=
)

if not "%PYTHONHOME%"=="" (
    echo Clearing PYTHONHOME, was set to %PYTHONHOME%
    set PYTHONHOME=
)

if "%PYTHONNOUSERSITE%"=="" (
    echo Setting PYTHONNOUSERSITE, was not set
    set PYTHONNOUSERSITE=True
)

:: Display version information
echo Using Python in %IDF_PYTHON_DIR%
%IDF_PYTHON% --version
echo Using Git in %IDF_GIT_DIR%
%IDF_GIT% --version

:: Set up DOSKEY macros for ESP-IDF commands
set PREFIX=%IDF_PYTHON% %IDF_PATH%
DOSKEY idf.py=%PREFIX%\tools\idf.py $*
DOSKEY esptool.py=%PREFIX%\components\esptool_py\esptool\esptool.py $*
DOSKEY espefuse.py=%PREFIX%\components\esptool_py\esptool\espefuse.py $*
DOSKEY espsecure.py=%PREFIX%\components\esptool_py\esptool\espsecure.py $*
DOSKEY otatool.py=%PREFIX%\components\app_update\otatool.py $*
DOSKEY parttool.py=%PREFIX%\components\partition_table\parttool.py $*

:: Run the export.bat if it exists
if exist "%IDF_PATH%\export.bat" call "%IDF_PATH%\export.bat"

:: Execute any commands passed as arguments
if "%~1"=="" (
    echo ESP-IDF environment is ready. You can now run ESP-IDF commands.
    cmd /k
) else (
    echo Executing command: %*
    %*
)
