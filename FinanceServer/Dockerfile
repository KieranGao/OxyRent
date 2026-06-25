# Stage 1: Build
FROM docker.cnb.cool/oxythecrack/docker-sync/ubuntu:22.04_amd64 AS builder

ENV DEBIAN_FRONTEND=noninteractive

# 配置DNS和国内镜像源
RUN echo "nameserver 8.8.8.8" > /etc/resolv.conf && \
    echo "nameserver 114.114.114.114" >> /etc/resolv.conf && \
    sed -i 's|http://archive.ubuntu.com|http://mirrors.tuna.tsinghua.edu.cn|g' /etc/apt/sources.list && \
    sed -i 's|http://security.ubuntu.com|http://mirrors.tuna.tsinghua.edu.cn|g' /etc/apt/sources.list

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

# 配置DNS和国内镜像源
RUN echo "nameserver 8.8.8.8" > /etc/resolv.conf && \
    echo "nameserver 114.114.114.114" >> /etc/resolv.conf && \
    sed -i 's|http://archive.ubuntu.com|http://mirrors.tuna.tsinghua.edu.cn|g' /etc/apt/sources.list && \
    sed -i 's|http://security.ubuntu.com|http://mirrors.tuna.tsinghua.edu.cn|g' /etc/apt/sources.list

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
COPY --from=builder /app/build/FinanceServer /usr/local/bin/FinanceServer

# Copy config
COPY config.ini /etc/server/config.ini

EXPOSE 50055

CMD ["FinanceServer", "--config", "/etc/server/config.ini"]
