#!/bin/bash

echo "Building Melvin HTTP Server..."
echo

# Check if gcc is available
if ! command -v gcc &> /dev/null; then
    echo "ERROR: gcc not found. Please install gcc."
    exit 1
fi

echo "Compiling melvin_server.c and melvin.c..."
gcc -o melvin_server melvin_server.c melvin.c -lm -std=c99 -Wall -pthread

if [ $? -eq 0 ]; then
    echo
    echo "Build successful! Run ./melvin_server to start the server."
    echo
else
    echo
    echo "Build failed! Check the errors above."
    echo
    exit 1
fi
