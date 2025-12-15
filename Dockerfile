# Dockerfile for Melvin Chat Interface (WebAssembly - Client-Side)
# WebAssembly files should be pre-built (melvin.js and melvin.wasm in web/)
# This just serves the static files

FROM nginx:alpine

# Copy web files (including pre-built WebAssembly)
COPY web/ /usr/share/nginx/html/

# Expose port
EXPOSE 80

# Nginx serves static files by default
