#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

echo "=== Building all C++ servers ==="

# GateServer needs jsoncpp at ../jsoncpp relative to its directory
# Copy it from OxyTeamTasks/jsoncpp if not present
if [ ! -d "$PROJECT_ROOT/jsoncpp" ] && [ -d "$PROJECT_ROOT/OxyTeamTasks/jsoncpp" ]; then
    echo "--- Setting up jsoncpp for GateServer ---"
    cp -r "$PROJECT_ROOT/OxyTeamTasks/jsoncpp" "$PROJECT_ROOT/jsoncpp"
fi

for server in GateServer UMSServer VehicleServer FinanceServer; do
    echo "--- Building $server ---"
    cd "$PROJECT_ROOT/$server"
    mkdir -p build
    cd build
    cmake ..
    make -j$(nproc)
    echo "--- $server built successfully ---"
done

cd "$PROJECT_ROOT"
echo "=== All servers built successfully ==="
echo "Binaries are in each server's build/ directory:"
echo "  GateServer/build/GateServer"
echo "  UMSServer/build/UMSServer"
echo "  VehicleServer/build/VehicleServer"
echo "  FinanceServer/build/FinanceServer"
