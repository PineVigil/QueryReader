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

REM ============================================
REM  Load user-local paths from paths.txt
REM  (QT_PREFIX, CMAKE_EXE). Edit paths.txt only!
REM ============================================
if exist "%SOURCE_DIR%\paths.txt" (
    for /f "usebackq tokens=1,* delims==" %%a in ("%SOURCE_DIR%\paths.txt") do (
        if /i "%%a"=="QT_PREFIX" set "QT_PATH=%%b"
        if /i "%%a"=="CMAKE_EXE" set "CMAKE_EXE=%%b"
    )
)

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

echo [3/3] Copying VC++ runtime DLLs to dist...
REM Qt and MuPDF depend on these; without them the exe crashes on startup
set SYS_DLLS=vcruntime140.dll msvcp140.dll vcruntime140_1.dll
for %%d in (%SYS_DLLS%) do (
    if not exist "%BUILD_DIR%\Release\%%d" (
        if exist "%QT_PATH%\bin\%%d" (
            copy "%QT_PATH%\bin\%%d" "%BUILD_DIR%\Release\" >nul 2>&1
            echo   copied %%d from Qt
        ) else if exist "C:\Windows\System32\%%d" (
            copy "C:\Windows\System32\%%d" "%BUILD_DIR%\Release\" >nul 2>&1
            echo   copied %%d from System32
        ) else (
            echo   WARNING: %%d not found
        )
    )
)

echo.
echo [DONE] Executable: "%BUILD_DIR%\Release\QueryReader.exe"
endlocal
