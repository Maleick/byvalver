FROM ubuntu:22.04

LABEL maintainer="umpolungfish"
LABEL description="byvalver - shellcode bad-byte elimination framework"

ENV DEBIAN_FRONTEND=noninteractive

# Install build dependencies
RUN apt-get update && apt-get install -y \
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
COPY Makefile decoder.asm ./
COPY src/ src/

# Build
RUN make release

# Copy remaining project files needed at runtime
COPY ml_models/ ml_models/
COPY verify_denulled.py verify_functionality.py verify_semantic.py ./

ENTRYPOINT ["./bin/byvalver"]
CMD ["--help"]
