#!/bin/bash
# Build and run C++ + CUDA + QASM hybrid example.
# Requires: qllvm, CUDA toolkit, qir-runner (pip install qirrunner)
# If CUDA is installed via apt, please first run: bash scripts/install_cuda_apt.sh
set -e
cd "$(dirname "$0")"

CUDA_ARCH="${CUDA_ARCH:-sm_75}"
CUDA_PATH="${CUDA_PATH:-}"
if [ -z "$CUDA_PATH" ] && [ -d "$HOME/.qllvm/cuda-apt-compat" ]; then
  export CUDA_PATH="$HOME/.qllvm/cuda-apt-compat"
fi

echo "Building hybrid_app (CUDA arch: $CUDA_ARCH, path: ${CUDA_PATH:-auto})..."
qllvm main.cpp kernel.cu circuit.qasm -o hybrid_app -cuda-arch "$CUDA_ARCH" \
  ${CUDA_PATH:+ -cuda-path "$CUDA_PATH"}

echo "Running..."
./hybrid_app -shots 1024
