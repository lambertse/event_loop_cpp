#!/bin/sh
cd build
cmake -DCMAKE_BUILD_TYPE=Release .. 
if [ $? -ne 0 ]; then
    echo "CMake configuration failed."
    exit 1
fi
cmake --build . --config Release
if [ $? -ne 0 ]; then
    echo "Build failed."
    exit 1
fi
