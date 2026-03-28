# qllvm Driver

This directory contains the main command-line driver script of qllvm.

## Files

| File | Description |
|------|-------------|
| `qllvm.in` | Driver script template; CMake `configure_file` generates the `qllvm` executable |
| `CMakeLists.txt` | Defines variables, runs `configure_file`, installs into `bin/` |

## Workflow

1. **Build time**: CMake substitutes `@VAR@` placeholders in `qllvm.in` with real paths (e.g. `@CMAKE_INSTALL_PREFIX@`, `@CLANG_EXECUTABLE@`), writes `build/qllvm`, and installs it to `CMAKE_INSTALL_PREFIX/bin/qllvm`.

2. **Run time**: When the user runs `qllvm`, the script dispatches to different compilation paths based on input file types (`.qasm`, `.cpp`, `.cu`):
   - QASM → invokes `qllvm-compile`
   - C++ + QASM → `compile_hybrid_qir_runner` / `compile_hybrid`
   - C++ + CUDA + QASM → `compile_hybrid_cuda_quantum`

## Relationship to qllvm-compile

- **qllvm** (this driver): High-level entry point; handles argument parsing, multi-file / multi-mode dispatch, and linking.
- **qllvm-compile** (`mlir/tools/`): Single-shot QASM → MLIR → QIR/backend compilation; invoked by `qllvm`.
