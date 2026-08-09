@echo off
setlocal

REM ============================================
REM  QueryReader build script (VS2022 generator)
REM ============================================

set QT_PATH=D:/app/qt/6.5.3/msvc2019_64
set SOURCE_DIR=%~dp0
set SOURCE_DIR=%SOURCE_DIR:~0,-1%
set BUILD_DIR=%SOURCE_DIR%\build
set CMAKE_EXE=D:\Program FilesMicrosoft Visual Studio2022Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe

echo [1/3] Configuring CMake (Visual Studio 2022 generator)...
"%CMAKE_EXE%" -S "%SOURCE_DIR%" -B "%BUILD_DIR%" -G "Visual Studio 17 2022" -A x64 "-DCMAKE_PREFIX_PATH=%QT_PATH%"
if errorlevel 1 (
    echo [ERROR] CMake configure failed
    exit /b 1
)

echo [2/3] Building...
REM MuPDF is a Release static library (/MD), must link with Release config
"%CMAKE_EXE%" --build "%BUILD_DIR%" --config Release
if errorlevel 1 (
    echo [ERROR] Build failed
    exit /b 1
)

echo.
echo [DONE] Executable: "%BUILD_DIR%\Release\QueryReader.exe"
endlocal