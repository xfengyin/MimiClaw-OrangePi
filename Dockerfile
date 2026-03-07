# MimiClaw-OrangePi Docker Build Environment
# Multi-stage build for optimized production image

# Build stage
FROM ubuntu:22.04 AS builder

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    libcurl4-openssl-dev \
    libjson-c-dev \
    libsqlite3-dev \
    libssl-dev \
    pkg-config \
    git \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /build

# Copy source code
COPY . /build/

# Build the application
RUN mkdir -p build && cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release && \
    make -j$(nproc)

# Runtime stage - minimal image
FROM ubuntu:22.04 AS runtime

# Install runtime dependencies only
RUN apt-get update && apt-get install -y \
    libcurl4 \
    libjson-c5 \
    libsqlite3-0 \
    libssl3 \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/* \
    && apt-get clean

# Create user and directories
RUN useradd -r -s /bin/false mimiclaw && \
    mkdir -p /opt/mimiclaw /etc/mimiclaw /var/lib/mimiclaw /var/log/mimiclaw && \
    chown -R mimiclaw:mimiclaw /var/lib/mimiclaw /var/log/mimiclaw

# Copy binary from builder
COPY --from=builder /build/build/mimiclaw /opt/mimiclaw/
COPY --from=builder /build/config/mimi_config.example.json /etc/mimiclaw/config.json

# Set permissions
RUN chmod +x /opt/mimiclaw/mimiclaw && \
    chown -R root:root /opt/mimiclaw /etc/mimiclaw

# Switch to non-root user
USER mimiclaw

# Expose ports
EXPOSE 8080 18789

# Health check
HEALTHCHECK --interval=30s --timeout=10s --start-period=5s --retries=3 \
    CMD curl -f http://localhost:8080/health || exit 1

# Run the application
ENTRYPOINT ["/opt/mimiclaw/mimiclaw"]
CMD ["--config", "/etc/mimiclaw/config.json"]