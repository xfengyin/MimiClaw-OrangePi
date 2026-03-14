# MimiClaw OrangePi - Docker Build

FROM arm32v7/ubuntu:22.04 AS builder

# Install build dependencies
RUN apt-get update && apt-get install -y \
    cmake \
    build-essential \
    libsqlite3-dev \
    libcurl4-openssl-dev \
    libssl-dev \
    libpthread-stubs0-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /build

# Copy source
COPY . .

# Build libraries and plugins
RUN make -C libs && \
    make -C plugins && \
    mkdir -p apps/mimi-gateway/build && \
    cd apps/mimi-gateway/build && \
    cmake ../.. -DCMAKE_BUILD_TYPE=Release && \
    make

# ============================================================================
# Runtime Image
# ============================================================================

FROM arm32v7/ubuntu:22.04

# Install runtime dependencies
RUN apt-get update && apt-get install -y \
    libsqlite3-0 \
    libcurl4 \
    libssl3 \
    && rm -rf /var/lib/apt/lists/* \
    && useradd -m -s /bin/bash mimi

WORKDIR /home/mimi

# Copy built artifacts
COPY --from=builder /build/libs/build/lib/*.so* ./lib/
COPY --from=builder /build/plugins/build/*.so ./plugins/
COPY --from=builder /build/apps/mimi-gateway/build/mimi-gateway ./bin/

# Copy config
COPY apps/mimi-gateway/config/gateway.json.in ./config/gateway.json

# Environment
ENV MIMI_API_KEY=""
ENV MIMI_MODEL="gpt-3.5-turbo"

# Entry point
ENTRYPOINT ["./bin/mimi-gateway"]
CMD ["-k", "${MIMI_API_KEY}"]