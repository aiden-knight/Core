@echo off

builder.exe build.cpp --config=tests
echo ----------------------------------------------------------------
echo.

.\\bin\\debug\\core-tests.exe
echo ----------------------------------------------------------------
echo.

builder.exe build.cpp --config=tests --release
echo ----------------------------------------------------------------
echo.

.\\bin\\release\\core-tests.exe
echo ----------------------------------------------------------------
echo.