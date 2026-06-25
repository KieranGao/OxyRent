# Stage 1: Build
FROM docker.cnb.cool/oxythecrack/docker-sync/ubuntu:22.04_amd64 AS builder

ENV DEBIAN_FRONTEND=noninteractive

# 使用国内镜像源
RUN sed -i 's|http://archive.ubuntu.com|http://mirrors.aliyun.com|g' /etc/apt/sources.list && \
    sed -i 's|http://security.ubuntu.com|http://mirrors.aliyun.com|g' /etc/apt/sources.list

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
FROM docker.cnb.cool/oxythecrack/docker-sync/ubuntu:22.04_amd64

ENV DEBIAN_FRONTEND=noninteractive

# 使用国内镜像源
RUN sed -i 's|http://archive.ubuntu.com|http://mirrors.aliyun.com|g' /etc/apt/sources.list && \
    sed -i 's|http://security.ubuntu.com|http://mirrors.aliyun.com|g' /etc/apt/sources.list

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
COPY --from=builder /app/build/UMSServer /usr/local/bin/UMSServer

# Copy config
COPY config.ini /etc/server/config.ini

EXPOSE 50053

CMD ["UMSServer", "--config", "/etc/server/config.ini"]
