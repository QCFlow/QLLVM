#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"

ARCH="${CUDA_ARCH:-sm_75}"
echo "Building mpi_cuda_app (arch=$ARCH) ..."
qllvm main_mpi_cuda.cpp kernel.cu circuit.qasm -o mpi_cuda_app \
  -qpu qir-runner -mpi -cuda-arch "$ARCH" -O1

NP="${1:-2}"
SHOTS="${2:-512}"
echo "Running: mpirun -np $NP ./mpi_cuda_app -shots $SHOTS"
mpirun -np "$NP" ./mpi_cuda_app -shots "$SHOTS"
