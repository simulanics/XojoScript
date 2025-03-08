@echo off
setlocal enabledelayedexpansion

:: Ensure the release\libs output directory exists
if not exist release-64 mkdir release-64
if not exist release-64\libs mkdir release-64\libs

:: Change to the Plugins directory
cd Plugins

:: --- Compile C++ Plugins in the current directory ---
for %%F in (*.cpp) do (
    set "filename=%%~nF"
    echo Compiling !filename!.cpp...
    g++ -shared -fPIC -o !filename!.dll %%F -m64
    move /Y !filename!.dll ..\release-64\libs\
)

:: --- Compile single-file Rust plugins in the current directory ---
for %%F in (*.rs) do (
    set "filename=%%~nF"
    echo Compiling !filename!.rs...
    rustc --crate-type=cdylib "%%F" -o !filename!.dll
    move /Y !filename!.dll ..\release-64\libs\
)

:: --- Build plugins in subdirectories that contain a build.bat file ---
for /D %%d in (*) do (
    if exist "%%d\build-64.bat" (
        echo Found build.bat in %%d, executing...
        pushd "%%d"
        call build-64.bat
        popd
        :: Move any DLL generated in this subdirectory to the release\libs folder
        for %%f in ("%%d\*.dll") do (
            echo Moving plugin DLL %%~nxf...
            move /Y "%%f" "..\release-64\libs\"
        )
    )
)

:: --- Recursively build Rust plugins (directories with Cargo.toml) ---
for /R %%F in (Cargo.toml) do (
    echo Found Cargo.toml in directory: %%~dpF
    pushd "%%~dpF"
    echo Building Rust plugin in %%~dpF...
    cargo build --release
    popd
    :: Move any DLL from the Rust plugin's target\release folder to the release\libs folder.
    for %%G in ("%%~dpFtarget\release\*.dll") do (
        echo Moving Rust plugin %%~nxG...
		echo "%%~dpFtarget\release\*.dll"
		echo "%%~G"
        xcopy /Y "%~dp0target\release\*.dll" "..\release-64\libs\"
    )
)

:: Return to the original directory
cd ..
echo Build complete.
pause
