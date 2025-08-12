#!/bin/bash
set -e

PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/build"
INSTALL_DIR="$PROJECT_ROOT/install"

# 1. Clean build & install
echo "==== Cleaning build & install directories ===="
rm -rf "$BUILD_DIR" "$INSTALL_DIR"

# 2. Build project (cross)
echo "==== Configuring project for Luckfox (cross) ===="
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"
cmake .. -DCROSS_COMPILE=ON

echo "==== Building project for Luckfox (cross) ===="
make -j$(nproc)

echo "==== Installing project for Luckfox (cross) ===="
make install DESTDIR="$INSTALL_DIR"

echo "==== Build & install for Luckfox completed! ====" 