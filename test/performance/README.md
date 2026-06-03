# Performance tests

Unified compile-time and circuit-metrics benchmarks across test suites and backends.

## Layout (planned)

| Path | Purpose |
|------|---------|
| `benchmarks.yaml` | Registry of suites (MQTBench, QASMBench, SupermarQ, …) |
| `../Performance_testing*.ipynb` | Legacy notebooks (optional; superseded by scripts) |

## Run (local)

```bash
./scripts/ci/run_perf_smoke.sh
```

Full matrix and reporting will live under `scripts/bench/` (Phase 2).
