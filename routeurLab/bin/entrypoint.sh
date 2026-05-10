#!/bin/bash
set -euo pipefail

HOSTNAME=$(hostname)
CFG_DIR="/etc/nodes"
CFG_FILE="${CFG_DIR}/${HOSTNAME}.cfg"
ETH1_INTERFACE="eth1"

echo "=== Node entrypoint: ${HOSTNAME} ==="

if [[ ! -f "${CFG_FILE}" ]]; then
    echo "[FATAL] Configuration file missing: ${CFG_FILE}"
    exit 1
fi

echo "[INFO] Loading config: ${CFG_FILE}"
source "${CFG_FILE}"

ip link set lo up
echo "[OK] Loopback interface up"

ip link set "${ETH1_INTERFACE}" up
echo "[OK] ${ETH1_INTERFACE} interface up"

ip addr flush dev "${ETH1_INTERFACE}" 2>/dev/null || true
ip addr add "${NODE_IP}/${NODE_PREFIX}" dev "${ETH1_INTERFACE}"
echo "[OK] IPv4 configured: ${NODE_IP}/${NODE_PREFIX}"

ip -6 addr add "${NODE_IP6}/${NODE_PREFIX6}" dev "${ETH1_INTERFACE}" nodad
echo "[OK] IPv6 configured: ${NODE_IP6}/${NODE_PREFIX6}"

echo ""
echo "[INFO] Network configuration:"
ip addr show
echo ""

if ping -c 2 -W 2 "${PEER_IP}" >/dev/null 2>&1; then
    echo "[OK] Peer IPv4 (${PEER_IP}) reachable"
else
    echo "[WARN] Peer IPv4 (${PEER_IP}) not reachable"
fi

if ping -6 -c 2 -W 2 "${PEER_IP6}" >/dev/null 2>&1; then
    echo "[OK] Peer IPv6 (${PEER_IP6}) reachable"
else
    echo "[WARN] Peer IPv6 (${PEER_IP6}) not reachable"
fi

echo "[INFO] Node ${HOSTNAME} ready"
