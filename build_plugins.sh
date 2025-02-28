#!/bin/bash

# Ensure the output directory exists
mkdir -p libs

# Change to the Plugins directory
cd Plugins || exit 1

# Loop through each .cpp file
for file in *.cpp; do
    filename="${file%.cpp}"  # Extract filename without extension
    echo "Compiling $file..."

    # Detect OS and set the correct output extension
    if [[ "$OSTYPE" == "darwin"* ]]; then
        g++ -shared -fPIC -o "$filename.dylib" "$file"  # macOS
        mv -f "$filename.dylib" ../libs/
    else
        g++ -shared -fPIC -o "$filename.so" "$file"  # Linux
        mv -f "$filename.so" ../libs/
    fi
done

# Return to the original directory
cd ..
echo "Build complete."
