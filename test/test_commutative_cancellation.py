#!/usr/bin/env python3
"""Semantic regressions for CommutativeCancellationPass.

Requires the existing qllvm Python environment (numpy, qiskit). Run:
  python test/test_commutative_cancellation.py --compiler build/bin/qllvm-compile

The isolated pipelines use pass IDs 2 (commutation) and 8 (unused-call cleanup).
Compare complete operators, up to global phase, AND terminal measurement wiring.
Default -O1 integration checks use the terminal-classical contract because its
separate diagonal-before-measurement pass intentionally changes the unitary.
"""

import argparse
import importlib.util
import json
from pathlib import Path
import subprocess
import tempfile
import time

import numpy as np
from qiskit import ClassicalRegister, QuantumCircuit, QuantumRegister, qasm2

ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "qllvm_verification", ROOT / "benchmark/official_suites/verification.py"
)
VERIFICATION = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(VERIFICATION)


def make_circuit(n, operations):
    circuit = QuantumCircuit(n, n)
    for name, args in operations:
        getattr(circuit, name)(*args)
    return circuit


def cases(random_count):
    result = {
        "cx_pair": make_circuit(2, [("cx", (0, 1))] * 2),
        "cx_odd_run": make_circuit(2, [("cx", (0, 1))] * 5),
        "cx_even_run": make_circuit(2, [("cx", (0, 1))] * 6),
        "rz_control": make_circuit(2, [
            ("cx", (0, 1)), ("rz", (0.37, 0)), ("cx", (0, 1))]),
        "rz_target": make_circuit(2, [
            ("cx", (0, 1)), ("rz", (0.37, 1)), ("cx", (0, 1))]),
        "rx_target": make_circuit(2, [
            ("cx", (0, 1)), ("rx", (0.37, 1)), ("cx", (0, 1))]),
        "rx_control": make_circuit(2, [
            ("cx", (0, 1)), ("rx", (0.37, 0)), ("cx", (0, 1))]),
        "rx_merge": make_circuit(1, [
            ("rx", (0.37, 0)), ("rx", (-0.22, 0)), ("rx", (0.81, 0))]),
        "rz_merge": make_circuit(1, [
            ("rz", (0.37, 0)), ("rz", (-0.22, 0)), ("rz", (0.81, 0))]),
        "h_pair": make_circuit(1, [("h", (0,)), ("h", (0,))]),
        "h_noncommuting": make_circuit(1, [
            ("h", (0,)), ("z", (0,)), ("h", (0,))]),
    }
    # Vary axes and operand relationships. Shared target alone must never imply
    # that controlled gates commute (e.g. CX(0,2), CZ(1,2), CX(0,2)).
    placements = {
        "same_control": ((0, 1), (0, 2)),
        "same_target": ((0, 2), (1, 2)),
        "opposite": ((0, 1), (1, 0)),
        "chain": ((0, 1), (1, 2)),
        "reverse_chain": ((1, 2), (0, 1)),
        "disjoint": ((0, 1), (2, 3)),
    }
    for first in ("cx", "cy", "cz"):
        for second in ("cx", "cy", "cz"):
            for placement, (a, b) in placements.items():
                n = max(*a, *b) + 1
                result[f"{first}_{second}_{placement}"] = make_circuit(
                    n, [(first, a), (second, b), (first, a)])

    # The shortcut must not treat SWAP as a controlled gate.
    result["cx_swap_shared_target"] = make_circuit(3, [
        ("cx", (0, 2)), ("swap", (1, 2)), ("cx", (0, 2))])
    a, b = QuantumRegister(2, "a"), QuantumRegister(2, "b")
    multi = QuantumCircuit(a, b, ClassicalRegister(4, "c"))
    multi.h(a[1])
    multi.cx(b[0], a[1])
    multi.rz(0.37, b[0])
    multi.cx(b[0], a[1])
    multi.rx(0.55, a[1])
    result["multiple_registers"] = multi

    for seed in range(random_count):
        rng = np.random.default_rng(seed)
        n = int(rng.integers(2, 5))
        circuit = QuantumCircuit(n, n)
        for _ in range(30):
            gate = rng.choice(["h", "x", "y", "z", "s", "t",
                               "rx", "ry", "rz", "cx", "cz", "cy"])
            if gate in ("cx", "cz", "cy"):
                i, j = rng.choice(n, 2, replace=False)
                getattr(circuit, gate)(int(i), int(j))
            elif gate in ("rx", "ry", "rz"):
                getattr(circuit, gate)(float(rng.uniform(-3, 3)),
                                       int(rng.integers(n)))
            else:
                getattr(circuit, gate)(int(rng.integers(n)))
        result[f"random_{seed}"] = circuit

    for circuit in result.values():
        circuit.measure(range(circuit.num_qubits), range(circuit.num_clbits))
    return result


EXPECTED_COUNTS = {
    "cx_pair": {"cx": 0}, "cx_odd_run": {"cx": 1},
    "cx_even_run": {"cx": 0}, "h_pair": {"h": 0},
    "cx_cx_same_control": {"cx": 1}, "cx_cx_same_target": {"cx": 1},
    "cx_cx_opposite": {"cx": 3}, "cx_cz_same_target": {"cx": 2},
    "rz_control": {"cx": 0}, "rz_target": {"cx": 2},
    "rx_target": {"cx": 0}, "rx_control": {"cx": 2},
    "rx_merge": {"rx": 1}, "rz_merge": {"rz": 1},
}


def run_case(compiler, directory, name, original, mode, timeout):
    stem = f"{name}_{mode}"
    source, output = directory / f"{stem}.qasm", directory / f"{stem}_out.qasm"
    source.write_text(qasm2.dumps(original) + "\n")
    output.unlink(missing_ok=True)
    cmd = [str(compiler), str(source), "-O1", "-qrt", "nisq",
           "-qpu", "qasm-backend", "-emit-backend=qasm-backend",
           f"-output-path={output}"]
    if mode != "full":
        cmd.append(f"-customPassSequence={directory / (mode + '.txt')}")
    row = {"case": name, "mode": mode, "command": cmd}
    start = time.monotonic()
    try:
        process = subprocess.run(cmd, capture_output=True, text=True,
                                 timeout=timeout, cwd=directory)
        row.update(returncode=process.returncode, wall_s=time.monotonic()-start)
        (directory / f"{stem}.stdout").write_text(process.stdout)
        (directory / f"{stem}.stderr").write_text(process.stderr)
        if process.returncode:
            raise RuntimeError(f"Compiler exited with {process.returncode}")
        compiled = qasm2.load(str(output),
                              custom_instructions=qasm2.LEGACY_CUSTOM_INSTRUCTIONS)
        result = VERIFICATION.verify(
            original, compiled, max_qubits=4, tolerance=1e-7,
            semantics="terminal-classical" if mode == "full" else "coherent")
        row.update(result)
        row["counts"] = dict(compiled.count_ops())
        if mode != "full" and result["status"] == "passed":
            for gate, count in EXPECTED_COUNTS.get(name, {}).items():
                actual = compiled.count_ops().get(gate, 0)
                if actual != count:
                    row.update(status="failed",
                               reason=f"Expected {count} {gate} gates, got {actual}")
    except Exception as exc:
        row.update(status="failed", reason=str(exc), wall_s=time.monotonic()-start)
    return row


def run_suite(args, directory):
    directory.mkdir(parents=True, exist_ok=True)
    for n in (1, 4):
        (directory / f"isolated{n}.txt").write_text("2\n" * n + "8\n")
    results = []
    for name, circuit in cases(args.random_cases).items():
        for mode in args.modes:
            row = run_case(args.compiler, directory, name, circuit, mode, args.timeout)
            results.append(row)
            print(f"{row['status']:6} {mode:10} {name} "
                  f"{row.get('reason', '')}", flush=True)
    (directory / "results.json").write_text(json.dumps(results, indent=2) + "\n")
    passed = sum(row["status"] == "passed" for row in results)
    print(f"{passed}/{len(results)} passed; results: {directory / 'results.json'}")
    return 0 if passed == len(results) else 1


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--compiler", type=Path,
                        default=ROOT / "build/bin/qllvm-compile")
    parser.add_argument("--work-dir", type=Path)
    parser.add_argument("--random-cases", type=int, default=40)
    parser.add_argument("--timeout", type=float, default=60)
    parser.add_argument("--modes", nargs="+", default=["isolated1", "isolated4"],
                        choices=["isolated1", "isolated4", "full"])
    args = parser.parse_args()
    args.compiler = args.compiler.resolve()
    if not args.compiler.is_file():
        parser.error(f"Compiler not found: {args.compiler}")
    if args.random_cases < 0:
        parser.error("--random-cases must be nonnegative")
    if args.work_dir:
        return run_suite(args, args.work_dir.resolve())
    with tempfile.TemporaryDirectory(prefix="qllvm-commutation-") as directory:
        return run_suite(args, Path(directory))


if __name__ == "__main__":
    raise SystemExit(main())
