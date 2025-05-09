@echo off
cd /D "%~dp0"
"F:/programming/wirender/build/bin/basic_test.exe"
if %errorlevel% neq 0 pause
