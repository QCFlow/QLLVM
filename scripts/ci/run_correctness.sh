#!/usr/bin/env bash
# CI/local entry: qllvm compiler correctness (small scale by default).
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "${ROOT}/test/correctness"

SCALE="${QLLVM_CORRECTNESS_SCALE:-small}"
OPT="${QLLVM_CORRECTNESS_OPT:-O0}"

echo "[ci] correctness: scale=${SCALE} opt=${OPT}"
python3 compiler_correctness_test.py --scale "${SCALE}" --opt "${OPT}" "$@"
