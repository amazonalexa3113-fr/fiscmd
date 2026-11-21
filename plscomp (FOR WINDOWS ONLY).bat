@echo off
setlocal

echo building the app for windows using g++ command
echo g++ -std=c++20 -Wall -Wextra -Wpedantic -Iinclude src/*.cpp -o fiscmd.exe

g++ -std=c++20 -Wall -Wextra -Wpedantic -Iinclude src/*.cpp -o fiscmd.exe

if errorlevel 1 (
    echo.
    echo BUILD FAILED
    exit /b 1
)

echo.
echo BUILD SUCCESSFUL
endlocal
