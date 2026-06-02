#!/usr/bin/env bash
# P0–P5 MPI hybrid compile + run tests
# Usage: bash scripts/test_hybrid_mpi.sh [all|sim|p5|p4|p3|p2]
set -euo pipefail

MODE="${1:-all}"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
QLLVM_BIN="${QLLVM_BIN:-$(command -v qllvm || true)}"

if [[ -z "$QLLVM_BIN" ]]; then
  if [[ -x "$ROOT/build/qllvm" ]]; then
    QLLVM_BIN="$ROOT/build/qllvm"
  elif [[ -x "$HOME/.qllvm/bin/qllvm" ]]; then
    QLLVM_BIN="$HOME/.qllvm/bin/qllvm"
  else
    echo "qllvm not found" >&2
    exit 1
  fi
fi

if ! command -v mpirun >/dev/null 2>&1; then
  echo "mpirun not found" >&2
  exit 1
fi

run_sim_p0() {
  local EX="$ROOT/examples/hybrid_mpi"
  cd "$EX"
  rm -f mpi_bell bell.bc __tmp_hybrid_*.qasm __qir_runner_mpi_kernels.* *_classical.o 2>/dev/null || true
  echo "=== [P0] MPI sim compile ==="
  "$QLLVM_BIN" main_mpi.cpp bell.qasm -o mpi_bell -qpu qir-runner -mpi -mpi-mode sim -O1
  out="$(mpirun -np 4 ./mpi_bell -shots 128 2>&1)"
  echo "$out" | grep -q "classical Allreduce"
  echo "$out" | grep -q "\[rank 3\]"
}

run_p5_rankmap() {
  local EX="$ROOT/examples/hybrid_mpi_rankmap"
  cd "$EX"
  rm -f mpi_rankmap bell.bc vqe.bc __tmp_hybrid_*.qasm __qir_runner_mpi_kernels.* *_classical.o 2>/dev/null || true
  echo "=== [P5] rank-kernel-map + parallel QASM compile ==="
  "$QLLVM_BIN" main_rankmap.cpp bell.qasm vqe.qasm -o mpi_rankmap \
    -qpu qir-runner -mpi -mpi-mode sim -rank-kernel-map 0:bell,1:vqe -O1 -v
  [[ -f bell.bc && -f vqe.bc ]]
  out="$(mpirun -np 2 ./mpi_rankmap -shots 64 2>&1)"
  echo "$out"
  echo "$out" | grep -q "dispatch_by_rank"
  echo "$out" | grep -q "\[rank 0\] histogram"
  echo "$out" | grep -q "\[rank 1\] histogram"
}

run_p4_llvm() {
  if ! command -v qir-runner >/dev/null 2>&1; then
    echo "SKIP P4: qir-runner not required but keeping sim deps check skipped"
  fi
  local EX="$ROOT/examples/hybrid_mpi_llvm"
  cd "$EX"
  rm -f mpi_llvm bell_compiled.qasm __hybrid_mpi_merged.* *_classical.ll *_qir.ll 2>/dev/null || true
  echo "=== [P4] MPI llvm-link mode ==="
  "$QLLVM_BIN" main_mpi_llvm.cpp bell.qasm -o mpi_llvm -mpi -mpi-mode llvm -O1 -v
  out="$(mpirun -np 2 ./mpi_llvm 2>&1)"
  echo "$out"
  echo "$out" | grep -q "llvm-link mode"
  echo "$out" | grep -q "in-process"
}

run_p3_hardware_mock() {
  local EX="$ROOT/examples/hybrid_mpi_hardware"
  cd "$EX"
  rm -f mpi_hw bell_compiled.qasm qllvm_mpir_mock_bell.json __mpiq_hardware_kernels.* *_classical.o 2>/dev/null || true
  echo "=== [P3] MPI hardware mock ==="
  "$QLLVM_BIN" main_mpi_hardware.cpp bell.qasm -o mpi_hw \
    -qpu qasm-backend -mpi -mpi-mode hardware -mpiq-config mock_config.json -O1 -v
  [[ -f bell_compiled.qasm ]]
  out="$(mpirun -np 2 ./mpi_hw 2>&1)"
  echo "$out"
  echo "$out" | grep -q "\[mpiq-mock\] rank 0 dispatched"
  echo "$out" | grep -q "\[mpiq-mock\] rank 1 synchronized"
  [[ -f qllvm_mpir_mock_bell.json ]]
}

run_p2_cuda() {
  if ! command -v nvcc >/dev/null 2>&1; then
    echo "SKIP P2: nvcc not found"
    return 0
  fi
  if ! command -v qir-runner >/dev/null 2>&1; then
    echo "qir-runner not found; pip install qirrunner" >&2
    exit 1
  fi
  local EX="$ROOT/examples/hybrid_mpi_cuda"
  cd "$EX"
  rm -f mpi_cuda_app circuit.bc __tmp_hybrid_*.qasm __qir_runner_mpi_kernels.* *_classical.o kernel_cuda.o 2>/dev/null || true
  CUDA_ARCH="${CUDA_ARCH:-sm_75}"
  echo "=== [P2] MPI+CUDA (arch=$CUDA_ARCH) ==="
  "$QLLVM_BIN" main_mpi_cuda.cpp kernel.cu circuit.qasm -o mpi_cuda_app \
    -qpu qir-runner -mpi -mpi-mode sim -cuda-arch "$CUDA_ARCH" -O1
  out="$(mpirun -np 2 ./mpi_cuda_app -shots 128 2>&1)"
  echo "$out" | grep -q "launching CUDA"
}

case "$MODE" in
  all)
    run_sim_p0
    run_p5_rankmap
    run_p4_llvm
    run_p3_hardware_mock
    run_p2_cuda
    ;;
  sim|p0) run_sim_p0 ;;
  p5) run_p5_rankmap ;;
  p4|llvm) run_p4_llvm ;;
  p3|hardware) run_p3_hardware_mock ;;
  p2|cuda) run_p2_cuda ;;
  *) echo "Unknown mode: $MODE (use all|sim|p5|p4|p3|p2)" >&2; exit 1 ;;
esac

echo ""
echo "ALL REQUESTED MPI TESTS PASSED (mode=$MODE)"
