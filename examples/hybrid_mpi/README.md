# MPI + QASM Hybrid Examples

QLLVM MPI hybrid compilation supports three modes via `-mpi-mode`:

| Mode | Flag | Backend | Runtime |
|------|------|---------|---------|
| **sim** (default) | `-mpi-mode sim -qpu qir-runner` | `.bc` + qir-runner | `libqllvm-mpi-runtime` |
| **llvm** | `-mpi-mode llvm` | llvm-link in-process | `libqir-qrt-stub` |
| **hardware** | `-mpi-mode hardware -qpu qasm-backend` | compile-time QASM | `libqllvm-mpiq-runtime` (mock) |

## P0: MPI + classical + QASM (sim)

```bash
cd examples/hybrid_mpi
qllvm main_mpi.cpp bell.qasm -o mpi_bell -qpu qir-runner -mpi -O1
mpirun -np 4 ./mpi_bell -shots 512
```

## P2: MPI + CUDA + QASM

See `examples/hybrid_mpi_cuda/`.

## P5: Per-rank kernel map + parallel QASM compile

```bash
cd examples/hybrid_mpi_rankmap
qllvm main_rankmap.cpp bell.qasm vqe.qasm -o mpi_rankmap \
  -qpu qir-runner -mpi -rank-kernel-map 0:bell,1:vqe -O1
mpirun -np 2 ./mpi_rankmap -shots 64
```

## P4: MPI + llvm-link (no subprocess)

```bash
cd examples/hybrid_mpi_llvm
qllvm main_mpi_llvm.cpp bell.qasm -o mpi_llvm -mpi -mpi-mode llvm -O1
mpirun -np 2 ./mpi_llvm
```

## P3: MPI hardware (mock MPIQ, no real QPU)

```bash
cd examples/hybrid_mpi_hardware
qllvm main_mpi_hardware.cpp bell.qasm -o mpi_hw \
  -qpu qasm-backend -mpi -mpi-mode hardware -mpiq-config mock_config.json -O1
mpirun -np 2 ./mpi_hw
# Produces bell_compiled.qasm + qllvm_mpir_mock_bell.json
```

When real hardware is available, replace mock runtime backend in `libqllvm-mpiq-runtime` with real MPIQ calls; the driver and stubs stay unchanged.

## Test suite

```bash
bash scripts/test_hybrid_mpi.sh all    # P0+P5+P4+P3+P2
bash scripts/test_hybrid_mpi.sh p5     # single phase
```
