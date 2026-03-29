#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
It converts the raw output of qir-runner to Qiskit-style counts statistics.

Usage:
  qir-runner -f bell.bc -s 100 | python scripts/qir_runner_counts.py
  python scripts/qir_runner_counts.py bell.bc -s 100 -r 42

Qiskit style output example:
  {'00': 48, '11': 52}
"""
import subprocess
import sys
from collections import Counter


def parse_qir_runner_output(text: str) -> list[list[int]]:
    """It parses the raw output of qir-runner, and returns the list of measurement results for each shot.

    Output format:
      START
      METADATA\tEntryPoint
      RESULT\t0     # the measurement result of the first qubit
      RESULT\t1     # the measurement result of the second qubit
      END\t0

    Returns: [[0, 1], [0, 0], [1, 1], ...]
    """
    shots = []
    results = []
    for line in text.splitlines():
        line = line.strip()
        if line.startswith("START"):
            results = []
        elif line.startswith("RESULT"):
            parts = line.split()
            if len(parts) >= 2:
                try:
                    results.append(int(parts[1]))
                except ValueError:
                    pass
        elif line.startswith("END"):
            if results:
                shots.append(results)
    return shots


def shots_to_counts(shots: list[list[int]]) -> dict[str, int]:
    """It converts the list of shots to a Qiskit-style counts dictionary."""
    bitstrings = ["".join(str(b) for b in shot) for shot in shots]
    return dict(Counter(bitstrings))


def run_qir_runner(bc_path: str, shots: int = 1024, seed: int | None = None) -> str:
    """It runs qir-runner and returns its stdout."""
    cmd = ["qir-runner", "-f", bc_path, "-s", str(shots)]
    if seed is not None:
        cmd.extend(["-r", str(seed)])
    return subprocess.run(cmd, capture_output=True, text=True).stdout


def main() -> None:
    # Mode 1: pipe input (qir-runner ... | python qir_runner_counts.py -)
    if len(sys.argv) >= 2 and sys.argv[1] == "-":
        text = sys.stdin.read()
        shots = parse_qir_runner_output(text)
        counts = shots_to_counts(shots)
        print(counts)
        return

    # Mode 2: python qir_runner_counts.py bell.bc [-s N] [-r SEED]
    if len(sys.argv) < 2 or not sys.argv[1].endswith(".bc"):
        print(__doc__, file=sys.stderr)
        sys.exit(1)

    bc_path = sys.argv[1]
    shots = 1024
    seed = None
    i = 2
    while i < len(sys.argv):
        if sys.argv[i] == "-s" and i + 1 < len(sys.argv):
            shots = int(sys.argv[i + 1])
            i += 2
        elif sys.argv[i] == "-r" and i + 1 < len(sys.argv):
            seed = int(sys.argv[i + 1])
            i += 2
        else:
            i += 1

    text = run_qir_runner(bc_path, shots, seed)
    shots_list = parse_qir_runner_output(text)
    counts = shots_to_counts(shots_list)
    print(counts)


if __name__ == "__main__":
    main()
