#!/bin/bash
LAB_NAME="e6-dns-latency"
IMAGE="clab-softnet-e6:latest"
TOPOLOGY="e6-dns-latency.clab.yml"
NODES="client dns-server"
LAB_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "${LAB_DIR}/../lib/deploy.sh"
