# Correctness tests

Compiler semantic correctness and fidelity checks.

## Layout

| Path | Purpose |
|------|---------|
| `compiler_correctness_test.py` | Main harness (MQTBench matrix) |
| `../MQTBench/` | Benchmark QASM inputs |
| `../qpu_configs/` | Backend topology configs |
| `../regression/` | Curated regressions for known bugs |

## Run (local)

```bash
# From repo root
./scripts/ci/run_correctness.sh

# Or directly
python test/correctness/compiler_correctness_test.py --scale small --opt O0
```

## CI

See `scripts/ci/run_correctness.sh` (PR: small; nightly: medium subset).
