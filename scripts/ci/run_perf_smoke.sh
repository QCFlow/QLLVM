#!/usr/bin/env bash
# CI/local entry: performance smoke (placeholder until bench registry lands).
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"

echo "[ci] perf smoke: not yet implemented (see test/performance/README.md)"
echo "[ci] planned: scripts/bench/run_matrix.py + test/performance/benchmarks.yaml"

# Optional: compile a single known-small circuit when qllvm is on PATH
if command -v qllvm >/dev/null 2>&1; then
  BELL="${ROOT}/test/test_bell.qasm"
  if [[ -f "${BELL}" ]]; then
    echo "[ci] compiling smoke circuit: ${BELL}"
    qllvm "${BELL}" -qrt nisq -qpu qasm-backend -O0 -o /tmp/qllvm_perf_smoke
    echo "[ci] perf smoke compile: ok"
    exit 0
  fi
fi

echo "[ci] skip: qllvm or test/test_bell.qasm not available"
exit 0
