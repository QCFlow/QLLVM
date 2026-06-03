# Docker build (planned)

Reproducible QLLVM build and test environment for `dev/foundation`.

## Goals

- **builder** stage: LLVM/MLIR, ANTLR, Eigen, project compile
- **runtime** stage: `qllvm`, minimal deps, `scripts/ci/*` smoke

## Placeholder usage (after Dockerfile lands)

```bash
docker build -t qllvm:foundation .
docker run --rm -v "$(pwd)":/src qllvm:foundation \
  bash -c 'cd /src && ./scripts/ci/run_correctness.sh'
```

Track implementation in Phase 0 of `docs/roadmap/foundation.md`.
