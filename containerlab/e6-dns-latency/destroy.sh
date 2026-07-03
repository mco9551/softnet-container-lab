#!/bin/bash
LAB_NAME="e6-dns-latency"
TOPOLOGY="e6-dns-latency.clab.yml"
LAB_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "${LAB_DIR}/../lib/destroy.sh"
