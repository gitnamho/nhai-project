@echo off
setlocal
cd /d "%~dp0"
echo Starting native WorkLookup with diagnostic logging...
ver
if not exist "%~dp0WorkLookup.exe" (
 echo WorkLookup.exe not found. Extract the entire ZIP first.
 pause
 exit /b 1
)
if exist "%APPDATA%\WorkLookup\native-startup.log" del /q "%APPDATA%\WorkLookup\native-startup.log"
start "" /wait "%~dp0WorkLookup.exe" /diagnose
set "WORKLOOKUP_EXIT=%ERRORLEVEL%"
echo Exit code: %WORKLOOKUP_EXIT%
if exist "%APPDATA%\WorkLookup\native-startup.log" (
 type "%APPDATA%\WorkLookup\native-startup.log"
) else (
 echo No startup log. The program entry point may not have been reached.
)
pause
exit /b %WORKLOOKUP_EXIT%
