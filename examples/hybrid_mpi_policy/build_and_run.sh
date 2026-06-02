#!/usr/bin/env bash
# Exp-2: MPI + CUDA + 8 policy circuits (Layout A: root quantum)
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
EX="$(cd "$(dirname "$0")" && pwd)"
STRONG="$ROOT/examples/hybrid_cuda_strong"
cd "$EX"

ARCH="${CUDA_ARCH:-sm_75}"
CUDA_PATH="${CUDA_PATH:-/usr/local/cuda}"

echo "=== Exp-2 Layout A: MPI + CUDA + policy_0..7 (root QPU) ==="
qllvm main_mpi_policy.cpp "$STRONG/kernel.cu" \
  "$STRONG/policy_0.qasm" "$STRONG/policy_1.qasm" "$STRONG/policy_2.qasm" \
  "$STRONG/policy_3.qasm" "$STRONG/policy_4.qasm" "$STRONG/policy_5.qasm" \
  "$STRONG/policy_6.qasm" "$STRONG/policy_7.qasm" \
  -o mpi_policy_app -qpu qir-runner -mpi -mpi-mode sim \
  -cuda-arch "$ARCH" -cuda-path "$CUDA_PATH" -O1

echo "=== mpirun -np 2 ==="
mpirun -np 2 ./mpi_policy_app --iters=2 --nx=64 --ny=64 -shots 128

echo "=== Exp-2b Layout B: rank-kernel-map (4 ranks, 4 policies) ==="
qllvm main_mpi_policy_rankmap.cpp "$STRONG/kernel.cu" \
  "$STRONG/policy_0.qasm" "$STRONG/policy_1.qasm" \
  "$STRONG/policy_2.qasm" "$STRONG/policy_3.qasm" \
  -o mpi_policy_rankmap -qpu qir-runner -mpi -mpi-mode sim \
  -rank-kernel-map 0:policy_0,1:policy_1,2:policy_2,3:policy_3 \
  -cuda-arch "$ARCH" -cuda-path "$CUDA_PATH" -O1

mpirun -np 4 ./mpi_policy_rankmap -shots 64

echo "Exp-2 OK"
