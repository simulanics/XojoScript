#!/bin/bash

# Compile xojoscript.cpp using g++
g++ -o xojoscript xojoscript.cpp -lffi -O3 -march=native -mtune=native -flto -m64 2> error.log

# Check if compilation was successful
if [ $? -ne 0 ]; then
    echo "Compilation failed! Check error.log for details."
    cat error.log
    exit 1
fi

# Ensure the release directory exists
mkdir -p release-64

# Move the compiled executable to the release directory
mv -f xojoscript release-64/

# Dump shared library dependencies based on OS and build XojoScript Embeddable Library
if [[ "$(uname)" == "Darwin" ]]; then
	g++ -o xojoscript.dylib xojoscript.cpp -lffi -O3 -march=native -mtune=native -flto -m64 2> errorlib.log
	mv -f xojoscript.dylib release-64/
    echo "Dylib dependencies:"
    otool -L release-64/xojoscript
elif [[ "$(uname)" == "Linux" ]]; then
	g++ -o xojoscript.so xojoscript.cpp -lffi -O3 -march=native -mtune=native -flto -m64 2> errorlib.log
	mv -f xojoscript.so release-64/
    echo "Shared library dependencies:"
    ldd release-64/xojoscript
else
    echo "Unsupported OS"
fi

# Copy the Scripts folder to the release directory
cp -r Scripts release-64/

echo "XojoScript Built Successfully."
exit 0
