#!/usr/bin/env bash

# build.sh
# Usage: ./build.sh
# Automatically detects macOS or Linux and builds accordingly.

if [ "$(uname -s)" = "Darwin" ]; then
  echo "Detected macOS. Building Shell.dylib..."
  g++ -s -shared -fPIC -m64 -O3 -o Shell.dylib Shell.cpp -pthread
  echo "Build complete: Shell.dylib"
else
  echo "Detected Linux. Building Shell.so..."
  g++ -s -shared -fPIC -m64 -O3 -o Shell.so Shell.cpp -pthread
  echo "Build complete: Shell.so"
fi
