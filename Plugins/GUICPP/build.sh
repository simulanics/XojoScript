#!/usr/bin/env bash

# build.sh
# Usage: ./build.sh
# Automatically detects macOS or Linux and builds accordingly.

if [ "$(uname -s)" = "Darwin" ]; then
  echo "Detected macOS. Building XGUI.dylib..."
  g++ -shared -fPIC -o XGUI.dylib XGUI.cpp $(pkg-config --cflags --libs gtk+-3.0)
  echo "Build complete: XGUI.dylib"
else
  echo "Detected Linux. Building XGUI.so..."
  g++ -shared -fPIC -o XGUI.so XGUI.cpp $(pkg-config --cflags --libs gtk+-3.0)
  echo "Build complete: XGUI.so"
fi
