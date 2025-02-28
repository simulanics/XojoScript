@echo off
setlocal

:: Compile resource file
windres xojoscript.rc -O coff -o xojoscript.res
if %ERRORLEVEL% NEQ 0 (
    echo Resource compilation failed! Check xojoscript.rc for errors.
    exit /b %ERRORLEVEL%
)

:: Compile xojoscript.cpp with metadata
g++ -o xojoscript.exe xojoscript.cpp xojoscript.res -lffi -O3 -march=native -mtune=native -flto 2> error.log

:: Check if compilation was successful
if %ERRORLEVEL% NEQ 0 (
    echo Compilation failed! Check error.log for details.
    type error.log
    exit /b %ERRORLEVEL%
)

echo XojoScript Built Successfully.
exit /b 0
