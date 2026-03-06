# MimiClaw-OrangePi Docker Build Environment
FROM ubuntu:22.04

# Install dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    gcc-arm-linux-gnueabihf \
    g++-arm-linux-gnueabihf \
    libc6-dev-armhf-cross \
    libcurl4-openssl-dev \
    libjson-c-dev \
    libsqlite3-dev \
    libssl-dev \
    pkg-config \
    git \
    wget \
    qemu-user-static \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /workspace

# Copy source
COPY . /workspace/

# Build
RUN mkdir -p build && cd build && \
    cmake .. \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_SYSTEM_NAME=Linux \
        -DCMAKE_SYSTEM_PROCESSOR=arm \
        -DCMAKE_C_COMPILER=arm-linux-gnueabihf-gcc \
        -DCMAKE_CXX_COMPILER=arm-linux-gnueabihf-g++ && \
    make -j$(nproc)

# Output
CMD ["cp", "build/mimiclaw", "/output/"]
