#!/bin/bash
set -e

PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/build"
INSTALL_DIR="$PROJECT_ROOT/install"

# 1. Clean build & install
echo "==== Cleaning build & install directories ===="
rm -rf "$BUILD_DIR" "$INSTALL_DIR"

# 2. Build project (native)
echo "==== Configuring project for Ubuntu 18.04 (native) ===="
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"
cmake .. -DCROSS_COMPILE=OFF

echo "==== Building project for Ubuntu 18.04 (native) ===="
make -j$(nproc)

echo "==== Installing project for Ubuntu 18.04 (native) ===="
make install DESTDIR="$INSTALL_DIR"

echo "==== Build & install for Ubuntu 18.04 completed! ====" 