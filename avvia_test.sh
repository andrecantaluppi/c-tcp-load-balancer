#!/bin/bash

# Stop execution if any command fails
set -e

echo "[*] Compiling load balancer..."
gcc -o balancer balancer.c -lpthread

echo "[*] Setting up mock backend servers..."
mkdir -p /tmp/backend1 /tmp/backend2 /tmp/backend3

# Create basic HTML responses
echo "<h1>Server 1 (Port: 8001)</h1>" > /tmp/backend1/index.html
echo "<h1>Server 2 (Port: 8002)</h1>" > /tmp/backend2/index.html
echo "<h1>Server 3 (Port: 8003)</h1>" > /tmp/backend3/index.html

# Start python http servers in background
python3 -m http.server 8001 --directory /tmp/backend1 > /dev/null 2>&1 &
P1=$!
python3 -m http.server 8002 --directory /tmp/backend2 > /dev/null 2>&1 &
P2=$!
python3 -m http.server 8003 --directory /tmp/backend3 > /dev/null 2>&1 &
P3=$!

# Ensure backends are killed when script exits
trap "echo '\n[*] Shutting down backends...'; kill $P1 $P2 $P3 2>/dev/null; exit" INT TERM EXIT

sleep 1

echo "[*] Setup complete. Test environment is running."
echo "[*] Load balancer address: http://127.0.0.1:8080"
echo "[*] Press Ctrl+C to stop all processes."
echo "---------------------------------------------------"

./balancer
