#!/bin/bash

# Compile xojoscript.cpp using g++
g++ -o xojoscript xojoscript.cpp -lffi -O3 -march=native -mtune=native -flto 2> error.log

# Check if compilation was successful
if [ $? -ne 0 ]; then
    echo "Compilation failed! Check error.log for details."
    cat error.log
    exit 1
fi

echo "XojoScript Built Successfully."
exit 0
