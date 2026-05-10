FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    iproute2 \
    iputils-ping \
    && rm -rf /var/lib/apt/lists/*


LABEL maintainer="clab-softnet-project"
LABEL description="Ubuntu 24.04 custom image for containerlab nodes with basic networking"

CMD ["sleep", "infinity"]
