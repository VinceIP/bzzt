@echo off
setlocal
rem ------------------------------------------------------------------
rem Configure + build
rem ------------------------------------------------------------------
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
if errorlevel 1 goto :end           rem stop if configure failed

cmake --build build -- -j8
if errorlevel 1 goto :end           rem stop if compile/link failed

rem ------------------------------------------------------------------
rem Run the game (Release build drops it in build\bzzt.exe)
rem ------------------------------------------------------------------
if exist "build\bzzt.exe" (
    echo.
    echo ===== Launching bzzt.exe =====
    echo.
    "build\bzzt.exe"
) else (
    echo ERROR: build\bzzt.exe not found – check CMake output paths.
)

:end
endlocal
