@echo off
cd /d "%~dp0"
if not exist "%~dp0WorkLookup.exe" (
 echo WorkLookup.exe not found. Extract the entire ZIP first.
 pause
 exit /b 1
)
start "" "%~dp0WorkLookup.exe"
if errorlevel 1 pause
