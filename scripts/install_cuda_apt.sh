#!/bin/bash
# Solution A: Install the CUDA Toolkit via apt, supporting mixed compilation of C++ + CUDA + QASM
# Usage: bash scripts/install_cuda_apt.sh
# Need: sudo permission
set -e

echo "[1/5] Check multiverse source..."
grep -q "deb.*multiverse" /etc/apt/sources.list /etc/apt/sources.list.d/*.list 2>/dev/null || {
    echo "Enable multiverse..."
    sudo add-apt-repository -y multiverse
}
sudo apt-get update

echo "[2/5] Install nvidia-cuda-toolkit..."
sudo apt-get install -y nvidia-cuda-toolkit

echo "[3/5] Check CUDA installation path..."
# apt layout: header files in /usr/include, libdevice in /usr/lib/nvidia-cuda-toolkit/libdevice
# Clang expectation: <cuda-path>/include, <cuda-path>/nvvm/libdevice, <cuda-path>/bin, <cuda-path>/lib64
CUDA_PATH=""
if [ -d /usr/local/cuda ]; then
    CUDA_PATH=/usr/local/cuda
    echo "Official CUDA detected: $CUDA_PATH"
elif [ -d /usr/lib/nvidia-cuda-toolkit ] && [ -f /usr/include/cuda_runtime.h ]; then
    echo "[4/5] Create Clang compatible directory structure (apt layout and Clang expectation are different)..."
    COMPAT="$HOME/.qllvm/cuda-apt-compat"
    mkdir -p "$COMPAT/nvvm"
    # Must use symlink instead of directory, cannot mkdir then ln (will become dir/xxx instead of top-level symlink)
    rm -rf "$COMPAT/include" "$COMPAT/nvvm/libdevice" "$COMPAT/bin" "$COMPAT/lib64"
    ln -sfn /usr/include "$COMPAT/include"
    ln -sfn /usr/lib/nvidia-cuda-toolkit/libdevice "$COMPAT/nvvm/libdevice"
    ln -sfn /usr/bin "$COMPAT/bin"
    for libdir in /usr/lib/x86_64-linux-gnu /usr/lib; do
        if [ -f "$libdir"/libcudart.so* ] 2>/dev/null; then
            ln -sfn "$libdir" "$COMPAT/lib64"
            break
        fi
    done
    [ -L "$COMPAT/lib64" ] || ln -sfn /usr/lib/x86_64-linux-gnu "$COMPAT/lib64"
    CUDA_PATH="$COMPAT"
    echo "Compatible layer created: $CUDA_PATH"
fi

if [ -z "$CUDA_PATH" ]; then
    echo "Warning: Cannot automatically detect CUDA path"
    CUDA_PATH="/usr/local/cuda"
fi

echo "[5/5] Verify installation..."
if command -v nvcc &>/dev/null; then
    echo "nvcc: $(nvcc --version 2>/dev/null | head -1)"
else
    echo "nvcc is in /usr/bin/nvcc"
fi

# Quick compile test    
QLLVM_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
echo ""
echo "Installation completed. Use the following commands to compile hybrid program:"
echo ""
echo "  export CUDA_PATH=$CUDA_PATH"
echo "  cd $QLLVM_ROOT/examples/hybrid_cuda"
echo "  qllvm main.cpp kernel.cu circuit.qasm -o hybrid_app -cuda-arch sm_75 -cuda-path \$CUDA_PATH"
echo ""
echo "Or directly:"
echo "  qllvm main.cpp kernel.cu circuit.qasm -o hybrid_app -cuda-arch sm_75 -cuda-path $CUDA_PATH"
echo ""
echo "Note: When nvcc is in PATH, qllvm will automatically use nvcc to compile .cu, no need for compatibility layer."
