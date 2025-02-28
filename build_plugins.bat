@echo off
setlocal enabledelayedexpansion

:: Ensure the output directory exists
if not exist libs mkdir libs

:: Change to the Plugins directory
cd Plugins

:: Loop through each .cpp file
for %%F in (*.cpp) do (
    set "filename=%%~nF"
    echo Compiling !filename!.cpp...
    
    :: Compile the plugin using g++
    g++ -shared -fPIC -o !filename!.dll !filename!.cpp
    
    :: Move the compiled DLL to the libs directory
    move /Y !filename!.dll ..\libs\
)

:: Return to the original directory
cd ..
echo Build complete.
pause
