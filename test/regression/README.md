# Regression suite

Small, hand-picked circuits that previously exposed compiler bugs. Each case should include:

- Input (`.qasm` or front-end source)
- Expected behavior (fidelity threshold, structural diff, or emit snapshot)
- Link to issue / commit that fixed the bug

Add new entries when fixing regressions found by `test/correctness/` or CI.
