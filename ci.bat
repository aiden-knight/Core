@echo off

builder.exe build.cpp --config=win64-debug-suc
echo ----------------------------------------------------------------
echo.

builder.exe build.cpp --config=win64-release-suc
echo ----------------------------------------------------------------
echo.

builder.exe build.cpp --config=win64-debug-non-suc
echo ----------------------------------------------------------------
echo.

builder.exe build.cpp --config=win64-release-non-suc
echo ----------------------------------------------------------------
echo.