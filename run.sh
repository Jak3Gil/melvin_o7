#!/bin/bash

echo "Starting Melvin HTTP Server..."
echo

if [ ! -f melvin_server ]; then
    echo "ERROR: melvin_server not found."
    echo "Please run ./build.sh first to compile the server."
    exit 1
fi

./melvin_server
