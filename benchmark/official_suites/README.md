# Optional official benchmark suites

QLLVM can download complete MQT Bench, QASMBench and Benchpress source snapshots
on demand. A normal Git clone contains only automation, tests, documentation and
version locks. Downloaded sources, generated circuits, catalogs, logs and results
are local files excluded from Git.

This uses a pinned download manifest rather than Git submodules. Each dependency
is pinned to an immutable upstream commit and an archive SHA-256 in
[sources.lock.json](sources.lock.json). Original generators, metrics, tests,
documentation and licenses are preserved when downloaded.

## Install and download

Use Python 3.11 for the current framework environment. From the QLLVM repository root:

```bash
# Create an isolated environment using the recorded dependency versions.
python3.11 -m venv .venv-bench
.venv-bench/bin/python -m pip install -r benchmark/official_suites/requirements.lock.txt

# List dependencies without network access.
.venv-bench/bin/python benchmark/official_suites/sources.py --list

# Download one suite, or omit --source to download all three.
.venv-bench/bin/python benchmark/official_suites/sources.py --fetch --source qasmbench
.venv-bench/bin/python benchmark/official_suites/sources.py --fetch

# Repeating --fetch verifies an existing snapshot; it never overwrites it.
.venv-bench/bin/python benchmark/official_suites/sources.py --verify

# Generate a local catalog using every official MQT algorithm family.
.venv-bench/bin/python benchmark/official_suites/prepare.py

# With only one suite downloaded:
.venv-bench/bin/python benchmark/official_suites/prepare.py --suite qasmbench \
  --output benchmark/official_suites/qasmbench.catalog.json
```

The three pinned repositories contain about 1 GB of source and data before
generating additional inputs. Downloads are explicit; normal benchmark execution
does not fetch dependencies or contact hardware.

A dependency refresh requires updating its commit, reference, archive URL and
SHA-256, file count and byte count together in sources.lock.json. Preserve the old
download outside upstream/ before fetching a new revision. An archive hash or
inventory mismatch fails before the new snapshot is installed.

## QLLVM compilation and framework comparison

```bash
# Direct official inputs, one suite at a time. Default profile: all catalog cases.
.venv-bench/bin/python benchmark/official_suites/run.py --suite qasmbench \
  --frameworks qllvm --input-mode original --repetitions 3 \
  --output build/benchmarks/qasmbench
.venv-bench/bin/python benchmark/official_suites/run.py --suite mqtbench \
  --frameworks qllvm --input-mode original --output build/benchmarks/mqtbench
.venv-bench/bin/python benchmark/official_suites/run.py --suite benchpress \
  --frameworks qllvm --input-mode original --output build/benchmarks/benchpress

# Small deterministic workflow check, using the same input for each framework.
.venv-bench/bin/python benchmark/official_suites/run.py --profile smoke \
  --frameworks qllvm qiskit tket pyqpanda pyqpanda3 pennylane \
  --input-mode canonical --repetitions 3 --output build/benchmarks/comparison

# Omit --profile smoke to run the full catalog.
# Repeat the identical command with --resume after an interruption.
```

Use --catalog for an alternate generated catalog and --compiler for a specific
QLLVM binary. Outputs must use a fresh directory; --resume requires the recorded
inputs, code and configuration. Put outputs under ignored build/ or outside the
checkout. summary.md, results.json, results.csv and per-case artifacts are generated.

Framework versions are pinned in versions.json; this is a dated release selection,
not an assertion that the versions remain latest indefinitely. setup.py
--refresh-versions explicitly queries PyPI. setup.py --environment PATH --native
installs those pins and writes an environment-specific dependency lock.
pyqpanda and pyqpanda3 are separate product lines.

## Suite-specific behavior

- **MQT Bench:** load the downloaded official generators and discover every family.
  Generate ALG, INDEP, NATIVEGATES and MAPPED inputs using official test sizes.
  --sizes, --levels, --gateset and --device change the finite parameter grid.
  Original QPY and exported QASM remain in ignored generated/. Dynamic circuits
  retain QASM3. A finite grid is not every possible generator parameterization.
- **QASMBench:** index every QASM/QASM3 file and archived QASM input in the complete
  source snapshot. Archive inputs are extracted only when selected. metrics.py
  runs the unchanged upstream QMetric implementation in a separate compatible environment.
- **Benchpress:** run.py covers official QASM inputs. native.py additionally runs
  the original parameterized workloads, generators, topology/gate-set validation,
  and pytest-benchmark metrics. QLLVM uses Qiskit only for circuit I/O and the
  official validation representation; compilation invokes the QLLVM CLI.
  Unsupported circuit-construction/manipulation SDK operations retain the
  official SKIPPED status.

```bash
# Native Benchpress workloads; optional --selection passes a pytest -k expression.
.venv-bench/bin/python benchmark/official_suites/native.py --frameworks qllvm \
  --output build/benchmarks/native-qllvm

# Official QASMBench metrics need legacy APIs; keep them outside the SDK environment.
python3.11 -m venv .venv-bench-metrics
.venv-bench-metrics/bin/python -m pip install \
  -r benchmark/official_suites/metrics-requirements.lock.txt
.venv-bench/bin/python benchmark/official_suites/metrics.py \
  --results build/benchmarks/comparison/results.json \
  --python .venv-bench-metrics/bin/python --output build/benchmarks/official-metrics

# Merge a completed framework retest only when inputs and settings match.
.venv-bench/bin/python benchmark/official_suites/report.py \
  --input build/benchmarks/comparison/results.json build/benchmarks/tket-retest/results.json \
  --replace tket --output build/benchmarks/combined
```

Some Conda-based Linux environments need a process-local libffi override for
pyqpanda imports. run.py and native.py accept --library-preload PATH and record
it; use only the library appropriate to that machine.

## Interpretation

The comparison uses shared opt0 u3/cx serialization, then each framework's own
optimizer. Original-input compilation and canonical-input comparison are distinct
runs. QLLVM uses O1, Qiskit opt3, tket FullPeephole/AutoRebase with implicit swaps
disabled, pyqpanda3 Transpiler opt2, pyqpanda circuit_optimizer and PennyLane compile.
Optimization levels across products do not imply equal effort.

Timing covers input loading, optimization and export, including QLLVM CLI startup.
SDK import, shared serialization and correctness checking are outside timing.
Independent warmup processes warm system caches, not persistent SDK compiler caches.
The default is one warmup and three recorded repetitions.

Performance comparisons require the same input hash and every repetition passing
the shared full-operator and terminal-measurement-map check. Standalone global
phase is allowed; controlled embedding and dynamic instrument equivalence need
different checks. Large circuits remain compile-only, never semantic passes.
--require-verification makes unverified selected inputs fail the run.

Gate-count rankings additionally require the common RX/RY/RZ/CZ/H/X/Y/Z output
basis. Unsupported inputs, shared-export failures, compiler failures, timeouts and
semantic failures stay visible. Native Benchpress PASSED follows its official
functional definition and does not imply full-operator equivalence.

Use --max-qubits, --max-bytes, --memory-gb and --timeout for resource limits.
Excluded cases remain in the report. Full native Benchpress execution may take
days. Complete source downloads and complete executed coverage are reported separately.

## Tests and debugging

Core tests run without downloading the optional suites:

```bash
python3 test/official_suites/test_suite.py
.venv-bench/bin/python test/official_suites/test_verification.py
.venv-bench/bin/python test/official_suites/test_adapters.py
```

The SDK tests require the framework environment. Native Benchpress tests require
its source download. [QLLVM debug TODO](../../docs/benchmark_debug_todo.md) records
the compiler follow-up work and acceptance criteria. Raw results and local review
materials are deliberately not part of the repository.
