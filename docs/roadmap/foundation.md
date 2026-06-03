# QLLVM foundation roadmap (`dev/foundation`)

Branch for base capabilities: reproducible builds, automated quality gates, language/backend coverage, and extensibility.

## Priority (P0 → P3)

| Priority | Work item | Notes |
|----------|-----------|--------|
| P0 | Docker reproducible build | `docs/build/docker.md` |
| P0 | Correctness automation | `test/correctness/`, `scripts/ci/run_correctness.sh` |
| P1 | OpenQASM3 completeness | Level A/B/C in `mlir/parsers/qasm3/` |
| P1 | Real hardware paths | `test/integration/`, qcis / tianyan / originquantum |
| P2 | Performance test matrix | `test/performance/`, `scripts/ci/run_perf_smoke.sh` |
| P2 | Complex Qiskit → MLIR | Extend `mlir/parsers/qiskit/` |
| P2 | Pluggable IR / Pass | Pipeline config before full plugins |
| P3 | QPanda → MLIR | No parser yet; evaluate export-first |
| P3 | HPC + advanced QPL | MPI/CUDA hybrid productization |

## Milestones

- **M1**: CI green on correctness `small`
- **M2**: OpenQASM3 Level A+B case set compiles
- **M3**: Backend emit smoke + docs for real hardware
- **M4**: One-command perf matrix report
- **M5**: New pass without editing `pass_manager.hpp` core switch

## Directory map

```
test/
  correctness/     README + links to harness
  performance/     README + future benchmarks.yaml
  integration/     backend / hardware smoke
  regression/      curated bug-fix cases
scripts/ci/
  run_correctness.sh
  run_perf_smoke.sh
docs/
  roadmap/foundation.md   (this file)
  build/docker.md         Docker placeholder
```

## Workflow

- Develop on `dev/foundation`; topic branches `dev/foundation/<topic>` for small PRs
- Merge to `master` via MR after CI
- Periodically: `git fetch origin && git merge origin/master`
