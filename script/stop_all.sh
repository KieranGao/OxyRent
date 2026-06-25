#!/bin/bash

echo "=== Stopping all services ==="

# Kill all server processes by binary name
for server in GateServer UMSServer VehicleServer FinanceServer; do
    pkill -f "$server --config" 2>/dev/null || true
done

echo "=== All services stopped ==="
