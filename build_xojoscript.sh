#!/bin/bash

# Compile xojoscript.cpp using g++
g++ -o xojoscript xojoscript.cpp -lffi -O3 -march=native -mtune=native -flto 2> error.log

# Check if compilation was successful
if [ $? -ne 0 ]; then
    echo "Compilation failed! Check error.log for details."
    cat error.log
    exit 1
fi

# Ensure the release directory exists
mkdir -p release

# Move the compiled executable to the release directory
mv -f xojoscript release/

# Dump shared library dependencies based on OS
if [[ "$(uname)" == "Darwin" ]]; then
    echo "Dylib dependencies:"
    otool -L release/xojoscript
elif [[ "$(uname)" == "Linux" ]]; then
    echo "Shared library dependencies:"
    ldd release/xojoscript
else
    echo "Unsupported OS"
fi

# Copy the Scripts folder to the release directory
cp -r Scripts release/

echo "XojoScript Built Successfully."
exit 0
