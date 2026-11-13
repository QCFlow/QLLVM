# Data archive for the npj Quantum Information manuscript

This directory is the data and code archive for the manuscript “QLLVM: An LLVM-Based Compilation Framework for Hybrid HPC-QC Programs.”

The QLLVM compiler source is the rest of this repository: <https://github.com/QCFlow/QLLVM>.

The tag `npj-qi-data-20261113` identifies this archive. The statements below are the Data Availability and Code Availability text in the manuscript.

## Data Availability

The records required to interpret and recompute the circuit-quality, semantic-validation, framework-comparison, routing, and public-workload results are available at https://github.com/QCFlow/QLLVM/tree/npj-qi-data-20261113/experiments/npj-qi-data. This directory contains the 430-circuit gate-count and depth table (scripts/data/paired_quality.csv); the semantic-validation archive of the 430 input and output circuits, evidence hashes, interval-certificate code, and calibration records (QLLVM_semantic_validation_430.zip); the Catalyst comparison records (QLLVM_quality_pilot_data.zip); the CUDA-Q, XACC, and configured-QCOR comparison records (QLLVM_framework_quality_data.zip); the 69-instance, three-seed routing records (QLLVM_routing_data.zip); and the public-workload build, runtime, and validation records (QLLVM_public_workload_records_20261103.zip). Running python3 scripts/check_evidence.py from that directory recomputes the 430-circuit Qiskit and Cirq means, the CUDA-Q, XACC, and configured-QCOR means, and the routing overheads, and checks the archived hashes. The hybrid-build and MPI timing results are reported as summary statistics in the article and its Supplementary Information. The complete per-run logs of those timing campaigns, and a compiler source snapshot that reproduces the evaluated binary, are not included. Before publication, one unused cloud API key was removed from a vendor-backend branch of scripts/qllvm_relocated.py inside the public-workload archive.

## Code Availability

The QLLVM source code is available at https://github.com/QCFlow/QLLVM. The scripts that audit the archived manuscript data and regenerate the circuit-quality figure from the paired quality table are in https://github.com/QCFlow/QLLVM/tree/npj-qi-data-20261113/experiments/npj-qi-data. The compiler binary used for the archived 430-circuit evaluation has SHA256 1449fbf775d63daab4c2eb747815018d278900510c9e9ea9ea5c383b2e12a60b. That binary and the dependency environment used to build it are not included in the repository.

## How to check the archives

From this directory, with Python 3 and only the standard library:

```bash
python3 scripts/check_evidence.py --output /tmp/npj-qi-evidence-audit.json
```

The expected full-set Qiskit means are a 4.77% gate-count reduction and a 4.08% depth reduction over 430 circuits. The expected routing overheads over 69 instances are 5.72% in gate count, 4.89% in depth, and 6.31% in two-qubit count. The CUDA-Q, XACC, and configured-QCOR means are taken from the stored normalized records: 54, 52, and 52 mutually verified pairs.

`scripts/plot_quality_figures.py` redraws the two circuit-quality panels from `scripts/data/paired_quality.csv`. It needs NumPy and Matplotlib. Figure 5 of the manuscript places the Qiskit panel beside the Cirq panel. `scripts/plot_supplement_build_figures.py` can redraw the build figures only when a build-summary CSV is supplied; that CSV is not part of this archive.

The semantic-validation zip contains `certify_ae.py`, `validate_certificate.py`, and `requirements-certificate.txt` for the 11 interval certificates and 28 calibration controls. Those checks need the packages named in that requirements file. They do not rerun the compiler.

## What is not in this archive

- Per-run logs for the 365 primary hybrid-build timing runs and 15 profiling builds.
- Per-run logs for the MPI timing campaigns.
- The frozen compiler binary identified by the SHA256 above, its build configuration, and its dependency environment.

The summary statistics for those timing experiments are in the manuscript and Supplementary Information.
