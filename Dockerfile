# Dockerfile for Melvin Chat Interface
FROM ubuntu:22.04

# Install build dependencies
RUN apt-get update && apt-get install -y \
    gcc \
    libc6-dev \
    make \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /app

# Copy source files
COPY melvin.c melvin_server.c ./
COPY web/ ./web/

# Compile the server
RUN gcc -o melvin_server melvin_server.c melvin.c -lm -std=c99 -Wall -O2

# Expose port
EXPOSE 8080

# Run the server
CMD ["./melvin_server"]
