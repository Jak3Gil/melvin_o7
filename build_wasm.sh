#!/bin/bash

echo "Building Melvin for WebAssembly..."
echo

# Check if emcc is available
if ! command -v emcc &> /dev/null; then
    echo "ERROR: emcc (Emscripten) not found."
    echo "Please install Emscripten: https://emscripten.org/docs/getting_started/downloads.html"
    exit 1
fi

echo "Compiling melvin.c and melvin_wasm.c to WebAssembly..."
emcc melvin.c melvin_wasm.c \
    -o web/melvin.js \
    -s WASM=1 \
    -s EXPORTED_RUNTIME_METHODS='["ccall","cwrap","UTF8ToString","stringToUTF8"]' \
    -s EXPORTED_FUNCTIONS='["_malloc","_free"]' \
    -s ALLOW_MEMORY_GROWTH=1 \
    -s INITIAL_MEMORY=16777216 \
    -O2 \
    -lm \
    -std=c99

if [ $? -eq 0 ]; then
    echo
    echo "Build successful! WebAssembly files created in web/ directory:"
    echo "  - melvin.js"
    echo "  - melvin.wasm"
    echo
    echo "The web interface is now ready for GitHub Pages deployment!"
else
    echo
    echo "Build failed! Check the errors above."
    exit 1
fi
