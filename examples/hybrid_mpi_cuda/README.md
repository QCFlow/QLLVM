# MPI + CUDA + QASM Hybrid (P2)

MPI program + CUDA kernel + OpenQASM circuit, unified `qllvm -mpi` build.

## Build

```bash
cd examples/hybrid_mpi_cuda
qllvm main_mpi_cuda.cpp kernel.cu circuit.qasm -o mpi_cuda_app \
  -qpu qir-runner -mpi -cuda-arch sm_80 -O1
```

Adjust `-cuda-arch` for your GPU (e.g. `sm_75`, `sm_80`).

## Run

```bash
mpirun -np 2 ./mpi_cuda_app -shots 512
```

## Test

```bash
bash ../../scripts/test_hybrid_mpi.sh
```
