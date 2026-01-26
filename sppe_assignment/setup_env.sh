#!/bin/bash
set -e

if command -v cpupower &>/dev/null; then
    cpupower frequency-set -g performance || true
elif command -v cpufreq-set &>/dev/null; then
    cpufreq-set -g performance || true
else
    echo "[WARN] cpupower/cpufreq-set not found, skipping CPU governor."
fi

sysctl -w vm.nr_hugepages=128

echo "[INFO] Applying TCP stack tuning..."
sysctl -w net.core.rmem_max=134217728
sysctl -w net.core.wmem_max=134217728
sysctl -w net.ipv4.tcp_rmem='4096 87380 134217728'
sysctl -w net.ipv4.tcp_wmem='4096 65536 134217728'
sysctl -w net.core.netdev_max_backlog=250000

cat /proc/cmdline
echo "---"
cat /proc/meminfo | grep Huge

echo "[INFO] HFT system tuning applied."
