# --- Stage 1: Build cross-compiler for i686-elf
FROM debian:stable-slim AS buildenv-builder
ARG DEBIAN_FRONTEND=noninteractive
ARG BINUTILS_VERSION=2.44
ARG GCC_VERSION=14.2.0
ARG PREFIX=/opt/cross
ARG TARGET=i686-elf

# Install build dependencies
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        build-essential bison flex libgmp-dev libmpc-dev libmpfr-dev texinfo \
        ca-certificates wget file python3 \
    && rm -rf /var/lib/apt/lists/*

# Build binutils
RUN cd /tmp && \
    wget -q https://ftp.gnu.org/gnu/binutils/binutils-${BINUTILS_VERSION}.tar.xz && \
    tar -xf binutils-${BINUTILS_VERSION}.tar.xz && \
    mkdir build-binutils && cd build-binutils && \
    ../binutils-${BINUTILS_VERSION}/configure \
      --target=${TARGET} \
      --prefix=${PREFIX} \
      --with-sysroot \
      --disable-nls \
      --disable-werror && \
    make -j"$(nproc)" && \
    make install && \
    rm -rf /tmp/*

# Build GCC
RUN cd /tmp && \
    wget -q https://ftp.gnu.org/gnu/gcc/gcc-${GCC_VERSION}/gcc-${GCC_VERSION}.tar.xz && \
    tar -xf gcc-${GCC_VERSION}.tar.xz && \
    mkdir build-gcc && cd build-gcc && \
    ../gcc-${GCC_VERSION}/configure \
      --target=${TARGET} \
      --prefix=${PREFIX} \
      --disable-nls \
      --disable-shared \
      --disable-threads \
      --disable-multilib \
      --disable-libssp \
      --disable-libgomp \
      --disable-libquadmath \
      --disable-libatomic \
      --disable-libstdcxx \
      --enable-languages=c \
      --without-headers && \
    make -j"$(nproc)" all-gcc all-target-libgcc && \
    make install-gcc install-target-libgcc && \
    rm -rf /tmp/*

# Extract i386-pc GRUB modules from grub-pc-bin amd64 package
RUN dpkg --add-architecture amd64 && \
    apt-get update && \
    cd /tmp && \
    apt-get download grub-pc-bin:amd64 && \
    ar x grub-pc-bin_*.deb data.tar.xz && \
    tar -xf data.tar.xz -C / ./usr/lib/grub/i386-pc && \
    rm -rf /tmp/* /var/lib/apt/lists/*

# --- Stage 2: Slim build environment with cross-compiler and GRUB tools
FROM debian:stable-slim AS buildenv
ARG DEBIAN_FRONTEND=noninteractive
ENV PATH=/opt/cross/bin:${PATH}
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        make libgmp-dev libmpc-dev libmpfr-dev python3 grub-common xorriso \
    && rm -rf /var/lib/apt/lists/*

# Copy cross toolchain and i386-pc GRUB modules from previous stage
COPY --from=buildenv-builder /opt/cross /opt/cross
COPY --from=buildenv-builder /usr/lib/grub/i386-pc /usr/lib/grub/i386-pc

WORKDIR /pacmanos
CMD ["/bin/bash"]
