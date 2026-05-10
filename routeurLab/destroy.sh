#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

echo "=== Destroy basic-lab ContainerLab Topology ==="

cd "${SCRIPT_DIR}"

echo ""
echo "== Step 1: Destroy topology =="
containerlab destroy -t basic-lab.clab.yml --cleanup
echo "[OK] Topology destroyed"

echo ""
echo "== Step 2: Remove Docker image? =="
if docker images clab-ubuntu-softnet:latest --format '{{.Repository}}:{{.Tag}}' | grep -q "clab-ubuntu-softnet:latest"; then
    read -r -p "Remove clab-ubuntu-softnet:latest image? [y/N] " answer
    if [[ "${answer}" =~ ^[Yy]$ ]]; then
        docker rmi clab-ubuntu-softnet:latest
        echo "[OK] Docker image removed"
    else
        echo "[INFO] Docker image kept"
    fi
fi

echo ""
echo "=== Cleanup Complete ==="
