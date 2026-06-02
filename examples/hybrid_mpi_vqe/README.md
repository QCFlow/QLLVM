# Exp-3: MPI VQE Outer Loop

MPI ranks run a classical reduction each iteration; all ranks call the VQE kernel (root executes + Bcast via MPI runtime).

```bash
bash build_and_run.sh
```

Uses `../hybrid/vqe.qasm`.
