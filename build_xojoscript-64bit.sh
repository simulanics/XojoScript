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

# Copy the Scripts folder to the release directory
cp -r Scripts release-64/

echo "XojoScript Built Successfully."
exit 0
