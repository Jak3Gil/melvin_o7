# Dockerfile for Melvin Chat Interface (WebAssembly - Client-Side)
FROM ubuntu:22.04

# Install build dependencies including Emscripten
RUN apt-get update && apt-get install -y \
    python3 \
    git \
    cmake \
    && rm -rf /var/lib/apt/lists/*

# Install Emscripten
RUN git clone https://github.com/emscripten-core/emsdk.git /opt/emsdk && \
    cd /opt/emsdk && \
    ./emsdk install latest && \
    ./emsdk activate latest

# Set working directory
WORKDIR /app

# Copy source files
COPY melvin.c melvin_wasm.c ./
COPY web/ ./web/

# Build WebAssembly
ENV PATH="/opt/emsdk:/opt/emsdk/upstream/emscripten:/opt/emsdk/node/current/bin:${PATH}"
RUN source /opt/emsdk/emsdk_env.sh && \
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

# Install a simple static file server
RUN apt-get update && apt-get install -y \
    python3-pip \
    && pip3 install http.server \
    && rm -rf /var/lib/apt/lists/*

# Expose port
EXPOSE 8080

# Serve static files (WebAssembly runs in browser)
CMD ["python3", "-m", "http.server", "8080", "--directory", "web"]
