#!/bin/bash

set -e  # Exit immediately if a command exits with a non-zero status

# Create build directory if it doesn't exist
mkdir -p build

# Enter build directory
cd build

# Configure for Release
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_POLICY_VERSION_MINIMUM=3.5 ..

# Build using all available processors
cmake --build . --config Release --parallel "$(sysctl -n hw.ncpu)"

# Go back to root directory
cd ..

# Ensure rootDir exists
mkdir -p rootDir

# Copy the built executable (macOS won't use .exe)
cp -f "build/src/Starlight" "rootDir/Starlight"

echo "Done. Output Path: rootDir/Starlight"