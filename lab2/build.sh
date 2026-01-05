#!/bin/bash

# Очистка
echo "Cleaning up..."
rm -rf build
mkdir build
cd build

# Сборка
echo "Building project..."
cmake ..
make

echo "Build complete. Executables are in 'build' directory."