#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$(dirname "$0")"

echo "=== Exp-3: MPI VQE outer loop ==="
qllvm main_mpi_vqe_loop.cpp "$ROOT/examples/hybrid/vqe.qasm" -o mpi_vqe_app \
  -qpu qir-runner -mpi -mpi-mode sim -O1

mpirun -np 4 ./mpi_vqe_app -shots 128
echo "Exp-3 OK"
