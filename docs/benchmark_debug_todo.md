# QLLVM benchmark debugging TODO

Status: open. These tasks arise from the pinned official-suite runs reviewed on
2026-10-27. They describe observed behavior and investigation steps, not established
root causes. Raw reports, downloaded benchmarks and generated outputs are kept
outside Git.

## Reproduce the environment first

- [ ] **P0 — establish binary/source provenance.** The observed compiler had SHA-256
  `1449fbf775d63daab4c2eb747815018d278900510c9e9ea5c383b2e12a60b`.
  The checkout had uncommitted compiler changes; HEAD alone cannot identify that
  executable's source. Rebuild the intended revision, record build configuration,
  source diff and binary hash, then rerun each case below with --compiler.
  Acceptance: a reproducible build and a documented O0/O1 baseline.

Download dependencies and prepare inputs as described in
[the benchmark guide](../benchmark/official_suites/README.md).
Commands below run from the repository root using the benchmark environment's
`python`. Use fresh output directories for each run.

## Compiler correctness and input support

- [ ] **P0 — equivalent gate spellings change the compiled operator.**
  The official Deutsch input passes when given directly, while its equivalent
  opt0 u3/cx serialization fails the strict operator check under O1.
  The shared serialization itself was checked for equivalence.
  QPT and iSWAP are additional spelling-sensitive candidates.
  Investigate u3 parameter order/signs, angle expressions, gate lowering,
  single-qubit merging and backend emission. These are hypotheses to isolate.

  ```bash
  python benchmark/official_suites/run.py --frameworks qllvm --opt O1 \
    --case qasmbench-small-deutsch_n2-deutsch_n2-qasm-5cbace8eac \
    --input-mode original --repetitions 1 --warmups 0 \
    --require-verification --output build/debug/deutsch-original
  python benchmark/official_suites/run.py --frameworks qllvm --opt O1 \
    --case qasmbench-small-deutsch_n2-deutsch_n2-qasm-5cbace8eac \
    --input-mode canonical --repetitions 1 --warmups 0 \
    --require-verification --output build/debug/deutsch-u3
  ```

  Repeat with --opt O0. Reduce to single-gate identities such as
  H = U3(pi/2, 0, pi), allowing standalone global phase. Bisect the pass sequence.
  Acceptance: both spellings preserve the complete operator and measurement
  destinations at O0/O1 within 1e-7; add the minimized case to compiler regression tests.

- [ ] **P0 — strict equivalence fails on additional static inputs.**
  DNN and quantumwalks fail even on original QASMBench inputs.
  MQT candidates include bmw_quark_cardinality, bmw_quark_copula, arithmetic
  circuits, graphstate, Grover, HHL and phase-estimation circuits.
  Do not assume all share one bug; cluster by first divergent IR/pass.
  Suitable initial cases:
  `qasmbench-small-dnn_n2-dnn_n2-qasm-205972d1ad`,
  `qasmbench-small-quantumwalks_n2-quantumwalks_n2-qasm-74013983bd`,
  `mqtbench-bmw_quark_cardinality-3-alg`.
  Use the command above with each ID, original input, and O0 then O1.
  Acceptance: minimized reproductions, pass-level diagnosis and strict
  full-operator regressions. Do not relax tolerances or switch contracts merely
  to turn an observed failure into a pass.

- [ ] **P1 — iterative phase estimation rejects a classical symbol.**
  Original `qasmbench-small-ipea_n2-ipea_n2-qasm-a221133cd8` exits with
  `[OPENQASM MLIRGen] Error: c is already in the symbol table.`
  Inspect classical declarations, measurement targets, reset and conditional
  symbol lookup. Compare original and canonical inputs to locate the frontend
  construct responsible.
  Acceptance: preserve reset/feedback and classical register semantics, or emit
  an explicit unsupported-feature diagnostic. Dynamic semantic correctness needs
  an instrument/channel checker; compilation alone does not close this task.

- [ ] **P1 — QASM3 dynamic input goes through an incompatible frontend path.**
  Official MQT `mqtbench-dynamic_qft-3-alg`, `mqtbench-ghz_dynamic-3-alg`
  and `mqtbench-iqpe-2-alg` are retained as QASM3 and fail with
  `no OPENQASM 2.0 preamble text.`
  Inspect version dispatch, accepted file extensions and supported control flow.
  Acceptance: route QASM3 to a supported path, or accurately reject the specific
  unsupported construct; never strip measurement, reset or feedback.
  Add version-dispatch and dynamic-semantics tests before claiming support.

## Compile cost and verification coverage

- [ ] **P1 — characterize multiplication compilation timeout.**
  `mqtbench-hrs_cumulative_multiplier-5-alg`, `-indep` and `-nativegates`
  exceeded a 10-second *whole-worker* limit. This is not yet evidence of an
  optimizer infinite loop: parsing, compilation and checking share that limit.
  Reproduce at 60/300 seconds, split phase timing, record peak memory and input
  gate count, then profile any dominant pass.
  Acceptance: explain the cost, bound the relevant phase and add a stable
  regression threshold after measuring the intended hardware.

- [ ] **P1 — define semantic contracts for reusable quantum outputs.**
  Earlier terminal-classical QFT/Toffoli/VQE examples preserve final measurement
  distributions while their full operators differ under O1. The official-suite
  runner now applies strict coherent checks. Determine whether each optimization
  is valid only for terminal measured outputs; constrain it using actual liveness
  and interface information.
  Acceptance: coherent-output tests and terminal-only tests remain distinct;
  controlled embedding uses a phase-exact contract when required.

- [ ] **P2 — extend correctness checking to dynamic and larger circuits.**
  Full-operator checking currently stops above its configured qubit limit;
  reset, feedback and repeated/intermediate measurement need a different method.
  Implement instrument/channel checks for small dynamic cases and choose a
  documented scalable verification strategy for larger inputs.
  Acceptance: verified, unsupported and compile-only remain distinct, and a
  compiler success or a favorable sample distribution cannot imply equivalence.

- [ ] **P2 — run wider regression and performance coverage after correctness fixes.**
  Execute the full pinned catalog, expand MQT sizes, and run the official
  Benchpress workload matrix. Preserve failures and resource exclusions.
  Record paired identical inputs, repeated timing distributions, common basis,
  hardware and build provenance.
  Acceptance: a new complete coverage report with justified resource limits;
  small smoke samples must not support suite-wide performance claims.

## Suggested order

1. Rebuild and identify the binary; reproduce original vs canonical Deutsch at O0/O1.
2. Minimize the static mismatches and bisect the first divergent pass.
3. Repair classical-symbol handling and QASM version dispatch.
4. Profile the multiplier timeout and extend verification coverage.
5. Rerun full suites and regenerate performance reports.

These tasks concern QLLVM. Observed tket or pyqpanda behavior belongs in separate
framework/adapter investigations and is not evidence of a QLLVM defect.
