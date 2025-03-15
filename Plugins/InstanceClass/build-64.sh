#!/usr/bin/env bash

# build.sh
# Usage: ./build.sh
# Automatically detects macOS or Linux and builds accordingly.

if [ "$(uname -s)" = "Darwin" ]; then
  echo "Detected macOS. Building InstanceClassDemo.dylib..."
  g++ -shared -fPIC -m64 -static -static-libgcc -static-libstdc++ -o InstanceClassDemo.dylib InstanceClassDemo.cpp -pthread
  echo "Build complete: InstanceClassDemo.dylib"
else
  echo "Detected Linux. Building InstanceClassDemo.so..."
  g++ -shared -fPIC -m64 -o InstanceClassDemo.so InstanceClassDemo.cpp -pthread
  echo "Build complete: InstanceClassDemo.so"
fi
