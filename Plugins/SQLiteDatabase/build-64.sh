#!/usr/bin/env bash

# build.sh
# Usage: ./build.sh
# Automatically detects macOS or Linux and builds accordingly.

if [ "$(uname -s)" = "Darwin" ]; then
  echo "Detected macOS. Building SQLiteDatabase.dylib..."
  g++ -s -shared -fPIC -m64 -O3 -o SQLiteDatabase.dylib SQLiteDatabase.cpp -lsqlite3
  echo "Build complete: SQLiteDatabase.dylib"
else
  echo "Detected Linux. Building SQLiteDatabase.so..."
  g++ -s -shared -fPIC -m64 -O3 -o SQLiteDatabase.so SQLiteDatabase.cpp -lsqlite3
  echo "Build complete: SQLiteDatabase.so"
fi
