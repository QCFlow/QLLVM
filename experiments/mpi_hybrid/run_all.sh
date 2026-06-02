#!/usr/bin/env bash
# Run recommended MPI experiments: Exp-1 build + Exp-2 policy + Exp-3 VQE
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
export PATH="${HOME}/.qllvm/bin:${PATH:-}"
export CUDA_ARCH="${CUDA_ARCH:-sm_75}"
export MPI_K_VALUES="${MPI_K_VALUES:-1,2,4,8}"
export MPI_SCAL_REPS="${MPI_SCAL_REPS:-2}"

echo "======== Exp-1: MPI build K-scan ========"
python3 "$ROOT/experiments/mpi_hybrid/run_mpi_experiments.py"
python3 "$ROOT/experiments/mpi_hybrid/plot_mpi_experiments.py"

echo "======== Exp-2: MPI+CUDA+multi-policy ========"
bash "$ROOT/examples/hybrid_mpi_policy/build_and_run.sh"

echo "======== Exp-3: MPI VQE outer loop ========"
bash "$ROOT/examples/hybrid_mpi_vqe/build_and_run.sh"

echo ""
echo "ALL MPI RECOMMENDED EXPERIMENTS PASSED"
