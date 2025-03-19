#!/usr/bin/env bash

# build.sh
# Usage: ./build.sh
# Automatically detects macOS or Linux and builds accordingly.

if [ "$(uname -s)" = "Darwin" ]; then
  echo "Detected macOS. Building LLMConnection.dylib..."
  g++ -shared -fPIC -m64 -O3 -o LLMConnection.dylib LLMConnection.cpp -lcurl
  echo "Build complete: LLMConnection.dylib"
else
  echo "Detected Linux. Building LLMConnection.so..."
  g++ -shared -fPIC -m64 -O3 -o LLMConnection.so LLMConnection.cpp -lcurl
  echo "Build complete: LLMConnection.so"
fi
