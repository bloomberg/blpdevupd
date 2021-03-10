@echo off
setlocal

REM ============== BUILD CONFIG ==============
set SRC_PATH=%~dp0\
set CMAKE_GENERATOR="Visual Studio 16 2019"
REM ==========================================

mkdir build
cd build

mkdir x86-debug
mkdir x86-release

echo "Starting project creation commands in parallel..."
(
start /D x86-debug cmake -G %CMAKE_GENERATOR% -A Win32 %SRC_PATH%
start /D x86-release cmake -G %CMAKE_GENERATOR% -A Win32 %SRC_PATH%
) | pause
echo "All done..."
