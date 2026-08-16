# ==========================================
# STAGE 1: Builder
# ==========================================
FROM ubuntu:24.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    ninja-build \
    git \
    curl \
    zip \
    unzip \
    tar \
    pkg-config \
    bison \
    flex \
    autoconf \
    automake \
    libtool \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /opt
RUN git clone https://github.com/microsoft/vcpkg.git && \
    ./vcpkg/bootstrap-vcpkg.sh -disableMetrics

ENV VCPKG_ROOT=/opt/vcpkg

WORKDIR /app

COPY vcpkg.json ./

RUN ${VCPKG_ROOT}/vcpkg install --triplet x64-linux

COPY . .

RUN cmake -B build -S . \
    -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE=${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake

RUN cmake --build build --config Release

# ==========================================
# STAGE 2: Runtime
# ==========================================
FROM ubuntu:24.04 AS runtime

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    ca-certificates \
    libstdc++6 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY --from=builder /app/build/runuram /app/runuram
COPY --from=builder /app/runuram.cfg /app/runuram.cfg

RUN mkdir -p /var/log/runuram /tmp/runuram /opt/runuram/ssl

EXPOSE 8080 8081 9090

CMD ["./runuram"]