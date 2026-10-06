@echo off
setlocal
cd /d "%~dp0"
i686-w64-mingw32-windres app.rc -o app-res.o
if errorlevel 1 exit /b 1
i686-w64-mingw32-g++ -std=c++17 -Os -municode -mwindows -static -static-libgcc -static-libstdc++ -ffunction-sections -fdata-sections -Wl,--gc-sections -Wl,--dynamicbase,--nxcompat -s src\main.cpp app-res.o -o WorkLookup.exe -lcomctl32 -lcomdlg32 -lshell32 -ladvapi32 -lgdi32 -luser32 -lole32
if errorlevel 1 exit /b 1
del app-res.o
echo Built native WorkLookup.exe. No .NET runtime required.
pause
