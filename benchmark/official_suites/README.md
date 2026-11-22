# Optional official benchmark suites

The suite compares QLLVM with pinned Qiskit, TKET, QPanda, QPanda3 and PennyLane versions on MQT Bench, QASMBench and Benchpress inputs. Input archives and SDK environments are downloaded explicitly; normal compiler builds do not require them.

## Prepare inputs

Use Python 3.11+ and create the SDK environment:

```bash
python3.11 benchmark/official_suites/setup.py --environment .venv-bench
.venv-bench/bin/python benchmark/official_suites/sources.py --fetch
.venv-bench/bin/python benchmark/official_suites/sources.py --verify
.venv-bench/bin/python benchmark/official_suites/prepare.py
```

`sources.lock.json` pins source commits and archive hashes; `versions.json` pins SDK versions. `--refresh-versions` is an explicit maintainer action. Downloaded inputs, generated catalogs, logs and per-run records remain ignored local data. Deterministic register repairs and the bounded single-bit feedback bridge preserve the original download and record corrected input hashes.

## Run a complete comparison

```bash
.venv-bench/bin/python benchmark/official_suites/run.py \
  --frameworks qllvm qiskit tket pyqpanda pyqpanda3 pennylane \
  --input-mode canonical --repetitions 3 --warmups 0 \
  --compiler build/bin/qllvm-compile --timeout 1800 --memory-gb 32 \
  --max-qubits 100000 --max-bytes 2147483648 \
  --output build/benchmarks/comparison
```

Use `--suite`, `--case` or `--profile smoke` for smaller runs. Use `--resume` to continue a matching run. Small-circuit equivalence checks are explicit and bounded; successful performance measurement is separate from semantic validation.

## Parallel QLLVM retest

Freeze a new QLLVM binary and reuse a completed campaign's canonical inputs and five-framework reference results:

```bash
.venv-bench/bin/python benchmark/official_suites/new_parallel_campaign.py \
  --reference /path/to/completed-campaign \
  --compiler build/bin/qllvm-compile --source-revision COMMIT \
  --python "$PWD/.venv-bench/bin/python" \
  --workers 8 --memory-budget-gb 64 \
  --output build/benchmarks/retest
.venv-bench/bin/python build/benchmarks/retest/parallel_runner.py \
  --campaign build/benchmarks/retest
```

Without `--reference`, use `--catalog` and optionally `--peers peer-reference.json` (`{"results": [...]}` containing no QLLVM records). `--preload` sets a platform-specific library only when needed. Each worker is pinned to one physical core, with native/SDK threads limited to one. Memory reservations cap parallelism; large inputs get longer timeouts. Each case records three real repetitions and independent checkpoints. Failures remain visible. Input SHA must match for comparisons; serial timing audits never replace parallel results.

## Build the webpage

```bash
python benchmark/official_suites/report/publish.py \
  --campaign build/benchmarks/retest --output build/benchmarks/report.html
```

The parallel runner also publishes `report.html` during and after a run. See [report/README.md](report/README.md) and the checked-in [report webpage](report/index.html). Only the final webpage snapshot is committed; raw test records, logs, SDKs and downloaded input repositories are not.
