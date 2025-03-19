@echo off
setlocal

:: Compile resource file
windres xcompile.rc -O coff -o xcompile.res
if %ERRORLEVEL% NEQ 0 (
    echo Resource compilation failed! Check xcompile.rc for errors.
    exit /b %ERRORLEVEL%
)

:: Compile xojoscript.cpp with metadata
g++ -s -static -m64 -o xcompile.exe xcompile.cpp xcompile.res -static-libgcc -static-libstdc++ -O3 -march=native -mtune=native 2> error.log

:: Check if compilation was successful
if %ERRORLEVEL% NEQ 0 (
    echo Compilation failed! Check error.log for details.
    type error.log
    exit /b %ERRORLEVEL%
)

:: Ensure the release directory exists
if not exist release-64 mkdir release-64

:: Move the compiled executable to the release directory
move /Y xcompile.exe release-64\

:: Dump DLL dependencies using objdump
echo DLL dependencies:
objdump -p release-64\xcompile.exe | findstr /R "DLL"

echo xcompile Built Successfully.
exit /b 0
