@echo off
echo Building Melvin HTTP Server...
echo.

REM Check if gcc is available
where gcc >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: gcc not found. Please install MinGW or another C compiler.
    pause
    exit /b 1
)

echo Compiling melvin_server.c and melvin.c...
gcc -o melvin_server.exe melvin_server.c melvin.c -lm -lws2_32 -std=c99 -Wall

if %ERRORLEVEL% EQU 0 (
    echo.
    echo Build successful! Run melvin_server.exe to start the server.
    echo.
) else (
    echo.
    echo Build failed! Check the errors above.
    echo.
    pause
    exit /b 1
)

pause
