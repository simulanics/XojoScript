@echo off
setlocal

:: Compile resource file
windres xojoscript.rc -O coff -o xojoscript.res
if %ERRORLEVEL% NEQ 0 (
    echo Resource compilation failed! Check xojoscript.rc for errors.
    exit /b %ERRORLEVEL%
)

:: Compile xojoscript.cpp with metadata (32-bit)
g++ -static -o xojoscript.exe xojoscript.cpp xojoscript.res -lffi -O3 -march=native -mtune=native 2> error.log

:: Check if compilation was successful
if %ERRORLEVEL% NEQ 0 (
    echo Compilation failed! Check error.log for details.
    type error.log
    exit /b %ERRORLEVEL%
)

:: Ensure the release directory exists
if not exist release mkdir release

:: Move the compiled executable to the release directory
move /Y xojoscript.exe release\

:: Dump DLL dependencies using objdump
echo DLL dependencies:
objdump -p release\xojoscript.exe | findstr /R "DLL"

:: Copy the Scripts folder to the release directory
xcopy /E /I /Y Scripts release\Scripts

echo XojoScript Built Successfully.
exit /b 0
