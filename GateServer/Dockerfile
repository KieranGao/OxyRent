# Stage 1: Build
FROM docker.cnb.cool/oxythecrack/docker-sync/ubuntu:22.04_amd64 AS builder

ENV DEBIAN_FRONTEND=noninteractive

# 使用国内镜像源
RUN sed -i 's|http://archive.ubuntu.com|http://mirrors.ustc.edu.cn|g' /etc/apt/sources.list && \
    sed -i 's|http://security.ubuntu.com|http://mirrors.ustc.edu.cn|g' /etc/apt/sources.list

RUN apt-get update && apt-get install -y \
    g++ cmake make \
    libboost-all-dev \
    libprotobuf-dev protobuf-compiler \
    libgrpc++-dev protobuf-compiler-grpc \
    libhiredis-dev \
    libmysqlcppconn-dev \
    pkg-config \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy entire project (build context is project root)
COPY . .

# GateServer CMakeLists.txt expects jsoncpp at ../jsoncpp relative to GateServer dir,
# which resolves to /app/jsoncpp. Copy it from OxyTeamTasks/jsoncpp.
RUN cp -r /app/OxyTeamTasks/jsoncpp /app/jsoncpp

# Build GateServer
WORKDIR /app/GateServer
RUN mkdir -p build && cd build && \
    cmake .. && \
    make -j$(nproc)

# Stage 2: Runtime
FROM docker.cnb.cool/oxythecrack/docker-sync/ubuntu:22.04_amd64

ENV DEBIAN_FRONTEND=noninteractive

# 使用国内镜像源
RUN sed -i 's|http://archive.ubuntu.com|http://mirrors.ustc.edu.cn|g' /etc/apt/sources.list && \
    sed -i 's|http://security.ubuntu.com|http://mirrors.ustc.edu.cn|g' /etc/apt/sources.list

RUN apt-get update && apt-get install -y \
    libboost-system1.74.0 \
    libboost-thread1.74.0 \
    libboost-filesystem1.74.0 \
    libprotobuf23 \
    libgrpc++1 \
    libhiredis0.14 \
    libmysqlcppconn7v5 \
    && rm -rf /var/lib/apt/lists/*

# Copy binary from builder
COPY --from=builder /app/GateServer/build/GateServer /usr/local/bin/GateServer

# Copy config
COPY GateServer/config.ini /etc/server/config.ini

EXPOSE 8080

CMD ["GateServer", "--config", "/etc/server/config.ini"]
