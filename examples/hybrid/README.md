# C++ + CUDA + QASM Hybrid Example

This example demonstrates a hybrid program that combines:
- C++ main program
- CUDA kernel (GPU computation)
- OpenQASM quantum circuit (Bell state)

## Files

- `main.cpp` - Main program; calls CUDA kernel and quantum circuit
- `kernel.cu` - CUDA device kernel and host launch function
- `circuit.qasm` - Bell state quantum circuit

## Compile with qllvm

```bash
qllvm main.cpp kernel.cu circuit.qasm -o hybrid_app \
      -cuda-arch sm_75 \
      -cuda-path /usr/local/cuda
```

- **`-cuda-arch`** - CUDA GPU architecture (e.g. `sm_75`, `sm_86`). Check your GPU with `nvidia-smi` or use `nvcc --help` for supported architectures.
- **`-cuda-path`** - Path to CUDA toolkit installation (optional if `CUDA_PATH` is set or CUDA is in the default location).

For qir-runner quantum backend:

```bash
qllvm main.cpp kernel.cu circuit.qasm -o hybrid_app \
      -qpu qir-runner \
      -cuda-arch sm_75 \
      -cuda-path /usr/local/cuda
```

## Run

```bash
./hybrid_app
```

## Requirements

- Clang with CUDA support (`clang -x cuda`)
- CUDA Toolkit (cudart, cuda, headers)
- qllvm-mlir-tool (for QASM compilation)
- qir-runner (optional, for quantum simulation: `pip install qirrunner`)

## Option A: Install CUDA via apt (Ubuntu)

If you install CUDA using `apt install nvidia-cuda-toolkit`, please first run the script provided by the project (it will create a Clang-compatible directory):

```bash
# Execute in qllvm root directory.
bash scripts/install_cuda_apt.sh
```

After installation, `build_and_run.sh` will automatically use `~/.qllvm/cuda-apt-compat`; or specify manually:

```bash
export CUDA_PATH=~/.qllvm/cuda-apt-compat
qllvm main.cpp kernel.cu circuit.qasm -o hybrid_app -cuda-arch sm_75 -cuda-path $CUDA_PATH
```
