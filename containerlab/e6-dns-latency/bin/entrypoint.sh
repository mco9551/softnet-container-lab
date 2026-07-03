#!/bin/bash
# Configure l'interface eth1 selon le fichier de config du node
set -euo pipefail

CFG="/etc/nodes/$(hostname).cfg"
if [ -f "$CFG" ]; then
    source "$CFG"
    ip addr add "${NODE_IP}/${NODE_PREFIX}" dev eth1
    ip link set eth1 up
fi
