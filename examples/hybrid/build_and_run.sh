#!/bin/bash
# Build and run C++ + OpenQASM hybrid example (no CUDA in this folder).
# Requires: qllvm, qir-runner for simulation (pip install qirrunner)
set -e
cd "$(dirname "$0")"

echo "Building hybrid_bell ..."
qllvm main.cpp bell.qasm -o hybrid_bell

echo "Running..."
./hybrid_bell
