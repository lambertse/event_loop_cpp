#!/bin/sh
mkdir build
cmake -DCMAKE_BUILD_TYPE=Debug -B build 
if [ $? -ne 0 ]; then
    echo "CMake configuration failed."
    exit 1
fi
cmake --build build 
if [ $? -ne 0 ]; then
    echo "Build failed."
    exit 1
fi
