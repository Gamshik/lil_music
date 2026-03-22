@echo off
setlocal

set "SCRIPT_DIR=%~dp0"
set "PROJECT_DIR=%SCRIPT_DIR%.."
set "CMAKE_EXE=C:\Program Files\Microsoft Visual Studio\18\Professional\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
set "PRESET=%~1"

if "%PRESET%"=="" set "PRESET=msvc-x64-debug"

if not exist "%CMAKE_EXE%" (
    echo Не найден CMake: "%CMAKE_EXE%"
    exit /b 1
)

pushd "%PROJECT_DIR%"
"%CMAKE_EXE%" --build --preset "%PRESET%"
set "EXIT_CODE=%ERRORLEVEL%"
popd

exit /b %EXIT_CODE%
