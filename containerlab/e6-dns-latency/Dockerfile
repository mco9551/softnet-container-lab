FROM ubuntu:24.04

COPY bin/entrypoint.sh /entrypoint.sh
RUN chmod +x /entrypoint.sh

RUN apt-get update && apt-get install -y \
    clang \
    llvm \
    libbpf-dev \
    iproute2 \
    linux-tools-common \
    dnsutils \
    iputils-ping \
    tcpdump \
    make \
    && rm -rf /var/lib/apt/lists/*

CMD ["sleep", "infinity"]
