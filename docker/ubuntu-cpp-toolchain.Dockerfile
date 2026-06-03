FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update -y && \
    apt-get install -y --no-install-recommends \
        build-essential \
        cmake \
        ninja-build \
        git \
        curl \
        python3 \
        python3-distutils \
        gcc \
        g++ \
        gcc-arm-linux-gnueabihf \
        g++-arm-linux-gnueabihf \
        gcc-aarch64-linux-gnu \
        g++-aarch64-linux-gnu \
        gcc-arm-none-eabi \
        g++-arm-none-eabi \
        binutils-arm-none-eabi && \
    rm -rf /var/lib/apt/lists/*

CMD ["/bin/bash"]
