@echo off
echo Starting Melvin HTTP Server...
echo.

if not exist melvin_server.exe (
    echo ERROR: melvin_server.exe not found.
    echo Please run build.bat first to compile the server.
    pause
    exit /b 1
)

melvin_server.exe

pause
