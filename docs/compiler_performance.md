# Compiler performance changes

This change set improves the QASM frontend, commutative cancellation and local synthesis, and adds a reproducible benchmark-to-webpage workflow.

- Explicit Release builds remain optimized. The frontend preserves expression precedence, exponent literals and unsigned 64-bit classical equality constants.
- Wire-aware commutation caches use exact parameters and operand roles; rewrite order follows causal SSA chains.
- KAK synthesis fixes Weyl coordinates, tensor order and phase representation. Matrix reconstruction and closed-block SSA checks guard replacements. The target synthesis supports zero through three CZ gates.
- Reset remains a nonunitary primitive through validation and basis lowering.
- Near-identity deletion keeps a cumulative error budget (default `1e-6`); exact mode disables nonzero deletions. Smaller certified charges are processed first.
- Optimization stops only after a complete unchanged IR snapshot; incomplete snapshots preserve the fixed four-round schedule.

The measured compiler snapshot is `cfaeaae`, with 100 compiler/semantic regressions and 224 mathematical checks passing. Small measured circuits use the appropriate operator or full quantum-instrument contract. Large benchmark completion does not imply exhaustive semantic verification.

The final 1058-case campaign (three repetitions each) obtained complete performance metrics for 1032 cases. Representative two-qubit counts changed as follows: DNN2 42→3, Heisenberg N4 48→12, Trotter N4 582→233, VQE4 88→71, IQPE2 2→1 and QV15 315→291. QFT75 improved 4471→3429 with the default error budget retained.

[Benchmark instructions](../benchmark/official_suites/README.md) explain downloads, pinned SDKs, bounded runs and report generation. The [self-contained report](../benchmark/official_suites/report/index.html) contains the final metrics and downloadable case table. Intermediate logs, old snapshots and process documents are intentionally excluded from this change set.

The retained upstream cancellation script additionally checks stronger gate-count goals. Its `cx_cx_same_control` case still emits three CX gates in isolated one-/four-pass modes (goal: one); the measured snapshot has the same limitation and both outputs remain operator-equivalent. This is a known optimization gap, not a semantic-validation pass or a new behavior introduced by regrouping commits.
