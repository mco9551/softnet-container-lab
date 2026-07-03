#!/bin/bash
CONTAINER="clab-e6-dns-latency-client"
IFACE="eth0"
OBJ="/tmp/dns_latency.bpf.o"
PIN="/sys/fs/bpf/dns_monitor"

# PID container for nsenter
CPID=$(docker inspect --format '{{.State.Pid}}' $CONTAINER)

echo "[1] Cleanup..."
sudo nsenter -t $CPID -n -- tc qdisc del dev $IFACE clsact 2>/dev/null || true
sudo mount -t bpf none /sys/fs/bpf 2>/dev/null || true
sudo rm -f $PIN

echo "[2] Copy .o from the container..."
docker cp $CONTAINER:/root/bpf/dns_latency.bpf.o $OBJ

echo "[3] Charge egress and pin the map..."
sudo nsenter -t $CPID -n -- tc qdisc add dev $IFACE clsact

echo "[4] Charge ingress by reusing the same map..."
sudo nsenter -t $CPID -n -- tc filter add dev $IFACE egress bpf da obj $OBJ sec tc verbose

echo "[5] Pin the loaded program..."
PROG_ID=$(sudo bpftool prog list | grep dns_monitor | tail -1 | awk '{print $1}' | tr -d ':')
echo "    Program ID = $PROG_ID"
sudo bpftool prog pin id $PROG_ID $PIN

echo "[6] Attach SAME program to ingress..."
sudo nsenter -t $CPID -n -- tc filter add dev $IFACE ingress bpf da pinned $PIN

echo "[OK] Done - One map per instance"
sudo bpftool map list | grep dns_timestamps
