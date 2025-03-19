@echo off
setlocal

:: Compile resource file
windres icoconvert.rc -O coff -o icoconvert.res
if %ERRORLEVEL% NEQ 0 (
    echo Resource compilation failed! Check icoconvert.rc for errors.
    exit /b %ERRORLEVEL%
)

:: Compile xojoscript.cpp with metadata
g++ icoconvert.cpp icoconvert.res -O2 -pipe -static -static-libstdc++ -static-libgcc -lgdiplus -s -o icoconvert.exe -march=native -mtune=native 2> error.log

:: Check if compilation was successful
if %ERRORLEVEL% NEQ 0 (
    echo Compilation failed! Check error.log for details.
    type error.log
    exit /b %ERRORLEVEL%
)

:: Ensure the release directory exists
if not exist release-64 mkdir release-64

:: Move the compiled executable to the release directory
move /Y icoconvert.exe release-64\

:: Dump DLL dependencies using objdump
echo DLL dependencies:
objdump -p release-64\icoconvert.exe | findstr /R "DLL"

echo xcompile Built Successfully.
exit /b 0
