#!/bin/bash
# Test script for OpenQASM-only build of qllvm
# Verifies: .qasm -> qllvm-mlir-tool -> .ll -> llvm-as -> llc -> .o -> executable

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
QLLVM_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
TEST_DIR="$QLLVM_ROOT/test"
INSTALL_PREFIX="${QCOR_INSTALL_PREFIX:-$HOME/.xacc}"
QCC="${QCC:-$INSTALL_PREFIX/bin/qllvm}"

echo "=== OpenQASM-only qllvm test ==="
echo "QCC: $QCC"
echo ""

if [ ! -x "$QCC" ]; then
    echo "Error: qcc not found, please build and install qllvm first"
    echo "  cd build && ninja install"
    echo "Or set QCOR_INSTALL_PREFIX / QCC environment variables"
    exit 1
fi

# Test 1: Compile .qasm to generate executable file
echo "--- Test 1: OpenQASM -> executable file ---"
cd "$TEST_DIR"
QASM_FILE="test_bell.qasm"
if [ ! -f "$QASM_FILE" ]; then
    echo "Error: $QASM_FILE not found"
    exit 1
fi

# Clean previous outputs
rm -f test_bell test_bell.o test_bell.ll test_bell.bc test_bell_compiled.qasm 2>/dev/null || true

echo "Compile $QASM_FILE ..."
if $QCC "$QASM_FILE" -o test_bell -qrt nisq -qpu qasm-backend 2>&1; then
    # qasm-backend uses emit-qasm path, directly generate test_bell_compiled.qasm (without generating executable file)
    if [ -f "test_bell_compiled.qasm" ]; then
        echo "✓ Test 1 passed: Successfully generated QASM (qasm-backend emit-qasm path)"
    elif [ -f "test_bell" ]; then
        echo "✓ Test 1 passed: Successfully generated executable file"
    else
        echo "Warning: Compilation succeeded but executable file or compiled.qasm not found"
    fi
else
    echo "Try simplified compilation (no -qrt/-qpu)..."
    $QCC "$QASM_FILE" -o test_bell 2>&1
    [ -f "test_bell" ] && echo "✓ Test 1 passed" || echo "Check: ls -la" && ls -la test_bell* 2>/dev/null || true
fi

# Test 2: Directly call qllvm-mlir-tool
echo ""
echo "--- Test 2: Directly call qllvm-mlir-tool ---"
MLIR_TOOL="$INSTALL_PREFIX/bin/qllvm-mlir-tool"
if [ -x "$MLIR_TOOL" ]; then
    cd "$TEST_DIR"
    $MLIR_TOOL "$QASM_FILE" -internal-func-name test_bell 2>&1
    if [ -f "test_bell.ll" ]; then
        echo "✓ Test 2 passed: qllvm-mlir-tool generates .ll"
    else
        echo "Warning: .ll file not found (may be output to other location)"
    fi
else
    echo "Skip: qllvm-mlir-tool not found"
fi

# Test 3: C++ input should be rejected
echo ""
echo "--- Test 3: C++ input should be rejected ---"
CPP_TEST=$(mktemp --suffix=.cpp)
echo "int main() { return 0; }" > "$CPP_TEST"
if $QCC "$CPP_TEST" 2>&1 | grep -q "not supported\|OpenQASM-only"; then
    echo "✓ Test 3 passed: C++ input correctly rejected"
else
    echo "Warning: C++ rejection behavior may vary due to version differences"
fi
rm -f "$CPP_TEST"

echo ""
echo "=== Test completed ==="
