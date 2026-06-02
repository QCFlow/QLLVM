# Exp-2: MPI + CUDA + Multi-Policy Quantum Control

Extends `hybrid_cuda_strong` to MPI multi-rank:

- **Layout A** (`mpi_policy_app`): rank 0 executes quantum; histogram broadcast to all ranks
- **Layout B** (`mpi_policy_rankmap`): `-rank-kernel-map 0:policy_0,1:policy_1,...`

```bash
bash build_and_run.sh
```

Requires: `qllvm`, `mpirun`, CUDA, `qir-runner`, policies in `../hybrid_cuda_strong/`.
