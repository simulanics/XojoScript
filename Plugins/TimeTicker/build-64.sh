#!/usr/bin/env bash

# build.sh
# Usage: ./build.sh
# Automatically detects macOS or Linux and builds accordingly.

if [ "$(uname -s)" = "Darwin" ]; then
  echo "Detected macOS. Building TimeTickerPlugin.dylib..."
  g++ -shared -fPIC -m64 -O3 -o TimeTickerPlugin.dylib TimeTickerPlugin.cpp -pthread
  echo "Build complete: TimeTickerPlugin.dylib"
else
  echo "Detected Linux. Building TimeTickerPlugin.so..."
  g++ -shared -fPIC -m64 -O3 -o TimeTickerPlugin.so TimeTickerPlugin.cpp -pthread
  echo "Build complete: TimeTickerPlugin.so"
fi
