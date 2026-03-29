#!/bin/bash
# qllvm + qir-runner collaborative test script for testing the integration of qllvm and qir-runner. 
# Usage: ./scripts/test_qllvm_qirrunner.sh [qllvm-mlir-tool path]
#        ./scripts/test_qllvm_qirrunner.sh -v build/bin/qllvm-mlir-tool  # debug mode
#
# Prerequisites:
#   1. conda qllvm environment installed qirrunner: pip install qirrunner
#   2. qllvm built (with qir-runner backend and QirRunnerCompat)

set -e

VERBOSE=0
QLLVM_TOOL=""
for a in "$@"; do
    case "$a" in
        -v|--verbose) VERBOSE=1 ;;
        *) [ -z "$QLLVM_TOOL" ] && QLLVM_TOOL="$a" ;;
    esac
done
QLLVM_TOOL="${QLLVM_TOOL:-$HOME/.qllvm/bin/qllvm-mlir-tool}"

# If the path is relative and executed from the qllvm root directory, complete the path
if [[ "$QLLVM_TOOL" == build/* ]] && [ -f "$(pwd)/$QLLVM_TOOL" ]; then
    QLLVM_TOOL="$(cd "$(dirname "$(pwd)/$QLLVM_TOOL")" && pwd)/$(basename "$QLLVM_TOOL")"
fi

if [ ! -f "$QLLVM_TOOL" ]; then
    QLLVM_TOOL="$HOME/qllvm/build/bin/qllvm-mlir-tool"
fi
if [ ! -f "$QLLVM_TOOL" ]; then
    echo "Error: qllvm-mlir-tool not found" >&2
    echo "  Try: ./scripts/test_qllvm_qirrunner.sh build/bin/qllvm-mlir-tool" >&2
    echo "  or ensure it is built: cd build && ninja" >&2
    exit 1
fi

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
QLLVM_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
TEST_DIR="$QLLVM_ROOT/examples/qasm"
OUT_DIR="/tmp/qllvm_qirrunner_test"
mkdir -p "$OUT_DIR"

[ $VERBOSE -eq 1 ] && echo "[DEBUG] QLLVM_ROOT=$QLLVM_ROOT TEST_DIR=$TEST_DIR OUT_DIR=$OUT_DIR"

if [ ! -f "$TEST_DIR/bell.qasm" ]; then
    echo "Error: $TEST_DIR/bell.qasm not found" >&2
    exit 1
fi

# Activate conda qllvm
if [ -f "$HOME/anaconda3/etc/profile.d/conda.sh" ]; then
    source "$HOME/anaconda3/etc/profile.d/conda.sh"
elif [ -f "/opt/conda/etc/profile.d/conda.sh" ]; then
    source /opt/conda/etc/profile.d/conda.sh
fi
if ! conda activate qllvm 2>/dev/null; then
    echo "Warning: conda activate qllvm failed, ensure qir-runner is in PATH" >&2
fi

if ! command -v qir-runner &>/dev/null; then
    echo "Error: qir-runner not found. Please: conda activate qllvm && pip install qirrunner" >&2
    exit 1
fi

echo "=== qllvm + qir-runner integration test ==="
echo "qllvm-mlir-tool: $QLLVM_TOOL"
[ $VERBOSE -eq 1 ] && echo "qir-runner: $(which qir-runner) $(qir-runner --version 2>/dev/null || true)"
echo ""

# Test 1: Bell circuit (with measurements)
echo "--- 1. Bell circuit (bell.qasm) ---"
[ $VERBOSE -eq 1 ] && echo "[DEBUG] compile: $QLLVM_TOOL $TEST_DIR/bell.qasm ..."
if ! "$QLLVM_TOOL" "$TEST_DIR/bell.qasm" -qrt nisq -qpu qir-runner -O1 \
    -emit-backend=qir-runner -output-path="$OUT_DIR/bell.bc" 2>&1; then
    echo "Error: qllvm-mlir-tool compilation failed" >&2
    exit 1
fi
[ $VERBOSE -eq 1 ] && echo "[DEBUG] run: qir-runner -f $OUT_DIR/bell.bc -s 3"
if ! qir-runner -f "$OUT_DIR/bell.bc" -s 3 2>&1; then
    echo "Error: qir-runner failed" >&2
    exit 1
fi
echo ""

# Test 2: No measurement circuit
echo "--- 2. No measurement circuit (bell_nomeasure.qasm) ---"
[ $VERBOSE -eq 1 ] && echo "[DEBUG] compile: $QLLVM_TOOL $TEST_DIR/bell_nomeasure.qasm ..."
if ! "$QLLVM_TOOL" "$TEST_DIR/bell_nomeasure.qasm" -qrt nisq -qpu qir-runner -O1 \
    -emit-backend=qir-runner -output-path="$OUT_DIR/bell_nm.bc" 2>&1; then
    echo "Error: qllvm-mlir-tool compilation failed" >&2
    exit 1
fi
[ $VERBOSE -eq 1 ] && echo "[DEBUG] run: qir-runner -f $OUT_DIR/bell_nm.bc -s 2"
if ! qir-runner -f "$OUT_DIR/bell_nm.bc" -s 2 2>&1; then
    echo "Error: qir-runner failed" >&2
    exit 1
fi
echo ""

echo "=== Integration test passed ==="
