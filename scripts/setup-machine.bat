@echo off
setlocal EnableExtensions

set "SCRIPT_DIR=%~dp0"
set "MANIFEST_FILE=%SCRIPT_DIR%..\tools\manifest.yaml"
set "MANIFEST_HELPER=%SCRIPT_DIR%manifest-helper.py"
set "TOOLS_ROOT=C:\tools"
set "CI_MODE=0"
set "SHOW_HELP=0"

:parse_args
if "%~1"=="" goto args_parsed
if "%~1"=="--ci" (
    set "CI_MODE=1"
    shift
    goto parse_args
)
if "%~1"=="--tools-root" (
    shift
    if "%~1"=="" goto arg_error
    set "TOOLS_ROOT=%~1"
    shift
    goto parse_args
)
if "%~1"=="--help" (
    set "SHOW_HELP=1"
    goto args_parsed
)
if "%~1"=="-h" (
    set "SHOW_HELP=1"
    goto args_parsed
)
echo Unknown option: %~1
goto usage

:arg_error
echo Missing value for --tools-root
exit /b 1

:args_parsed
if "%SHOW_HELP%"=="1" goto usage
if not exist "%MANIFEST_HELPER%" (
    echo Manifest helper not found: %MANIFEST_HELPER%
    exit /b 1
)

set "PYTHON="
for %%P in (python python3) do (
    where %%P >nul 2>&1
    if not errorlevel 1 if "%PYTHON%"=="" set "PYTHON=%%P"
)

if "%PYTHON%"=="" (
    echo Python is required to run this script.
    exit /b 1
)

if "%CI_MODE%"=="1" (
    echo CI mode: verifying prerequisites in the current container or image.
    "%PYTHON%" "%MANIFEST_HELPER%" check "%MANIFEST_FILE%"
    exit /b %ERRORLEVEL%
)

echo Tools root: %TOOLS_ROOT%
"%PYTHON%" "%MANIFEST_HELPER%" check "%MANIFEST_FILE%"
if errorlevel 1 (
    echo.
    echo If tools are not installed system-wide, add %TOOLS_ROOT%\bin to PATH or install Visual Studio Build Tools.
)
exit /b %ERRORLEVEL%

:usage
echo Usage: %~nx0 [--ci] [--tools-root PATH] [--help]
echo.
echo   --ci           Verify prerequisites in a CI/container environment.
echo   --tools-root   Path used for shared tool install advice (default C:\tools).
echo   --help         Show this help message.
exit /b 0
