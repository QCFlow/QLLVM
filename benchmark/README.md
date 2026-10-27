# QLLVM benchmarks

[official_suites](official_suites/README.md) provides optional, pinned downloads of
complete MQT Bench, QASMBench and Benchpress sources, QLLVM compilation automation,
native metrics, and cross-framework comparisons.

A normal clone contains scripts and dependency locks. Fetch the suites explicitly
with sources.py --fetch. Downloaded sources, generated inputs and runtime results
are ignored; write results under build/benchmarks/ or outside the checkout.

See [benchmark debug TODO](../docs/benchmark_debug_todo.md) for compiler issues
identified by these workflows. The suite covers compilation; it does not certify
hardware execution, hybrid runtimes or fault-tolerant execution.
