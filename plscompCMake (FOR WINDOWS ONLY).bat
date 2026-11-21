@echo off
setlocal

echo building the app for windows using cmake
echo cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

if errorlevel 1 (
    echo CMake configuration FAILED
    exit /b 1
)

echo cmake --build build --config Release

cmake --build build --config Release

if errorlevel 1 (
    echo BUILD FAILED
    exit /b 1
)

echo.
echo BUILD SUCCESSFUL
endlocal
