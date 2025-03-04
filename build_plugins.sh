#!/bin/bash

# Ensure the release/libs output directory exists
mkdir -p release/libs

# Change to the Plugins directory
cd Plugins || { echo "Plugins directory not found"; exit 1; }

##############################
# Compile C++ Plugins in the current directory
##############################
for file in *.cpp; do
    [ -e "$file" ] || continue
    filename="${file%.cpp}"
    echo "Compiling $file..."
    if [[ "$OSTYPE" == "darwin"* ]]; then
        g++ -shared -fPIC -o "$filename.dylib" "$file"
        mv -f "$filename.dylib" ../release/libs/
    else
        g++ -shared -fPIC -o "$filename.so" "$file"
        mv -f "$filename.so" ../release/libs/
    fi
done

##############################
# Compile single-file Rust plugins in the current directory
##############################
for file in *.rs; do
    [ -e "$file" ] || continue
    filename="${file%.rs}"
    echo "Compiling $file..."
    if [[ "$OSTYPE" == "darwin"* ]]; then
        rustc --crate-type=cdylib "$file" -o "$filename.dylib"
        mv -f "$filename.dylib" ../release/libs/
    else
        rustc --crate-type=cdylib "$file" -o "$filename.so"
        mv -f "$filename.so" ../release/libs/
    fi
done

##############################
# Build plugins in subdirectories that contain a build.sh file
##############################
for dir in */ ; do
    if [ -f "$dir/build.sh" ]; then
        echo "Found build.sh in $dir, executing..."
        pushd "$dir" > /dev/null || continue
        bash build.sh
        popd > /dev/null
        if [[ "$OSTYPE" == "darwin"* ]]; then
            lib_ext="dylib"
        else
            lib_ext="so"
        fi
        for lib in "$dir"*."$lib_ext"; do
            [ -e "$lib" ] || continue
            echo "Moving $(basename "$lib") to ../release/libs/"
            mv -f "$lib" ../release/libs/
        done
    fi
done

##############################
# Recursively build Rust plugins (directories containing Cargo.toml)
##############################
find . -name Cargo.toml | while read -r cargo_file; do
    plugin_dir=$(dirname "$cargo_file")
    echo "Found Cargo.toml in directory: $plugin_dir"
    pushd "$plugin_dir" > /dev/null || continue
    echo "Building Rust plugin in $plugin_dir..."
    cargo build --release
    popd > /dev/null
    if [[ "$OSTYPE" == "darwin"* ]]; then
        lib_ext="dylib"
    else
        lib_ext="so"
    fi
    if [ -d "$plugin_dir/target/release" ]; then
        for lib in "$plugin_dir/target/release/"*."$lib_ext"; do
            [ -e "$lib" ] || continue
            echo "Moving $(basename "$lib") to ../release/libs/"
            mv -f "$lib" ../release/libs/
        done
    fi
done

cd .. || exit 1
echo "Build complete."
