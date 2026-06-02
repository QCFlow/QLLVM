# MPI Hybrid Recommended Experiments

Three experiments highlighting QLLVM MPI co-compilation vs manual split pipelines.

## Exp-1: MPI build scalability (K-scan)

Compare **unified** `qllvm -mpi` vs **manual MPI split** (mpicxx + nvcc + K× qllvm-compile + MPI stub).

```bash
export PATH="$HOME/.qllvm/bin:$PATH"
export CUDA_PATH=/usr/local/cuda   # adjust
export CUDA_ARCH=sm_75
export MPI_K_VALUES="1,2,4,8,16"
python3 run_mpi_experiments.py
python3 plot_mpi_experiments.py
```

Outputs:
- `results/mpi_build_scalability.csv`
- `results/figures/fig_mpi_build_scalability.pdf`

## Exp-2: MPI + CUDA + multi-policy HPC loop

Layout **A** (root QPU + histogram Bcast): `examples/hybrid_mpi_policy/build_and_run.sh`

Layout **B** (rank-kernel-map): same script, second binary `mpi_policy_rankmap`

Uses 8× `policy_*.qasm` from `examples/hybrid_cuda_strong/`.

## Exp-3: MPI VQE outer loop

Classical MPI Allreduce + root quantum kernel each iteration:

```bash
bash examples/hybrid_mpi_vqe/build_and_run.sh
```

## Run all

```bash
bash experiments/mpi_hybrid/run_all.sh
```

## Paper narrative

| Exp | Claim |
|-----|--------|
| Exp-1 | Unified `-mpi` build stays ~flat in K; split grows with K (stub + serial qllvm-compile) |
| Exp-2 | Single ELF deploys MPI+HPC+CUDA+8 quantum policies; rank-map for heterogeneous ranks |
| Exp-3 | Iterative hybrid (VQE-style) without Python orchestration or runtime `qllvm-compile` |

LaTeX draft: `../../paper/mpi_hybrid_section.tex` (optional)
