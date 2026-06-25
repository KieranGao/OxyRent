#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

echo "=== Starting all services ==="

# Start in dependency order (databases first, then gRPC servers, then gateway)

echo "Starting UMSServer..."
cd "$PROJECT_ROOT/UMSServer" && ./build/UMSServer --config config.ini &
sleep 2

echo "Starting VehicleServer..."
cd "$PROJECT_ROOT/VehicleServer" && ./build/VehicleServer --config config.ini &
sleep 2

echo "Starting FinanceServer..."
cd "$PROJECT_ROOT/FinanceServer" && ./build/FinanceServer --config config.ini &
sleep 2

echo "Starting GateServer..."
cd "$PROJECT_ROOT/GateServer" && ./build/GateServer --config config.ini &
sleep 2

echo "=== All services started ==="
echo "GateServer (HTTP): http://localhost:8080"
echo "UMSServer (gRPC):  localhost:50053"
echo "VehicleServer (gRPC): localhost:50054"
echo "FinanceServer (gRPC): localhost:50055"
echo ""
echo "Press Ctrl+C to stop all services"

# Wait for Ctrl+C
wait
