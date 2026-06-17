#!/usr/bin/env bash
set -e

BUILD_DIR="build-win"
# Windows-native destination — avoids GPU driver issues with \\wsl$\ network paths
WIN_DEST="/mnt/c/cubeworld"

echo "=== CubeWorld — cross-compile to Windows ==="
cmake -S . -B "$BUILD_DIR" \
    -DCMAKE_TOOLCHAIN_FILE=cmake/mingw-toolchain.cmake \
    -DCMAKE_BUILD_TYPE=Release

cmake --build "$BUILD_DIR" -- -j"$(nproc)"

echo ""
echo "=== Copying to C:\\cubeworld\\ ==="
mkdir -p "$WIN_DEST"
cp "$BUILD_DIR/cubeworld.exe" "$WIN_DEST/"
cp -r "$BUILD_DIR/assets" "$WIN_DEST/"

echo ""
echo "=== Done! ==="
echo "Run the game from Windows:"
echo "   C:\\cubeworld\\cubeworld.exe"
