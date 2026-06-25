# Stage 1: Build
FROM ubuntu:22.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    g++ cmake make \
    libboost-all-dev \
    libprotobuf-dev protobuf-compiler \
    libgrpc++-dev protobuf-compiler-grpc \
    libhiredis-dev \
    libmysqlcppconn-dev \
    libssl-dev \
    pkg-config \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy source code
COPY . .

# Build
RUN mkdir -p build && cd build && \
    cmake .. && \
    make -j$(nproc)

# Stage 2: Runtime
FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    libboost-system1.74.0 \
    libboost-thread1.74.0 \
    libboost-filesystem1.74.0 \
    libprotobuf23 \
    libgrpc++1 \
    libhiredis0.14 \
    libmysqlcppconn8 \
    libssl3 \
    && rm -rf /var/lib/apt/lists/*

# Copy binary from builder
COPY --from=builder /app/build/VehicleServer /usr/local/bin/VehicleServer

# Copy config
COPY config.ini /etc/server/config.ini

EXPOSE 50054

CMD ["VehicleServer", "--config", "/etc/server/config.ini"]
