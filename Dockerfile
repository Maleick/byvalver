FROM ubuntu:22.04

LABEL maintainer="umpolungfish"
LABEL description="byvalver - shellcode bad-byte elimination framework"

ENV DEBIAN_FRONTEND=noninteractive

# Install build dependencies
RUN apt-get update && apt-get install -y \
    binutils \
    gcc \
    make \
    nasm \
    xxd \
    pkg-config \
    libcapstone-dev \
    libncurses-dev \
    python3 \
    && rm -rf /var/lib/apt/lists/*

# Copy source
WORKDIR /opt/byvalver
COPY . .

# Build
RUN make clean && make release

ENTRYPOINT ["./bin/byvalver"]
CMD ["--help"]
