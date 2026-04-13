# This code is part of QLLVM.
#
# (C) Copyright QCFlow 2026.
#
# This code is licensed under the Apache License, Version 2.0. You may
# obtain a copy of this license in the LICENSE file in the root directory
# of this source tree or at https://www.apache.org/licenses/LICENSE-2.0.
#
# Any modifications or derivative works of this code must retain this
# copyright notice, and modified files need to carry a notice indicating
# that they have been altered from the originals.

"""
Benchmark QLLVM vs Qiskit, Cirq, PennyLane: gate count, depth, compile time, peak memory.

Metrics for gate count / depth: load the compiled circuit as Qiskit QuantumCircuit when
possible so numbers are comparable. Native basis sets differ slightly; see docstrings.
"""

from __future__ import annotations

import os
import re
import shutil
import subprocess
import tempfile
import threading
import time
import tracemalloc
import warnings
from dataclasses import dataclass
from typing import Any, Callable, Optional

from qiskit import QuantumCircuit, transpile
from qiskit.qasm2 import dumps as qasm2_dumps

# Optional: Cirq (needs ply for OpenQASM import)
try:
    import cirq
    from cirq.contrib.qasm_import import circuit_from_qasm

    _CIRQ_AVAILABLE = True
except Exception:  # pragma: no cover
    _CIRQ_AVAILABLE = False

try:
    import pennylane as qml

    _PL_AVAILABLE = True
except Exception:  # pragma: no cover
    _PL_AVAILABLE = False

# Default Qiskit / QLLVM basis (aligned with Performance_testing.ipynb)
BASIS_GATES = ["rx", "ry", "rz", "cz", "h"]
QISKIT_OPTIMIZATION_LEVEL = 3


@dataclass
class BenchResult:
    """Single run metrics for one benchmark."""

    backend: str
    qasm_path: str
    gate_count: Optional[int] = None
    depth: Optional[int] = None
    compile_time_s: Optional[float] = None
    memory_peak_bytes: Optional[int] = None
    error: Optional[str] = None
    notes: str = ""


def qiskit_metrics_from_qasm(qasm_str: str) -> tuple[int, int]:
    qc = QuantumCircuit.from_qasm_str(qasm_str)
    return qc.size(), qc.depth()


def qiskit_metrics_from_file(path: str) -> tuple[int, int]:
    qc = QuantumCircuit.from_qasm_file(path)
    return qc.size(), qc.depth()


def _apply_qiskit_instruction_to_qml(inst, qc: QuantumCircuit) -> None:
    """Emit PennyLane ops for one Qiskit instruction (unitary gates only)."""
    op = inst.operation
    name = op.name.lower()
    qs = [qc.find_bit(q).index for q in inst.qubits]
    params = list(getattr(op, "params", []) or [])

    if name in ("barrier", "measure"):
        return
    if name == "id":
        return
    if name == "h":
        qml.Hadamard(wires=qs[0])
    elif name == "x":
        qml.PauliX(wires=qs[0])
    elif name == "y":
        qml.PauliY(wires=qs[0])
    elif name == "z":
        qml.PauliZ(wires=qs[0])
    elif name == "s":
        qml.S(wires=qs[0])
    elif name == "sdg":
        qml.adjoint(qml.S)(wires=qs[0])
    elif name == "t":
        qml.T(wires=qs[0])
    elif name == "tdg":
        qml.adjoint(qml.T)(wires=qs[0])
    elif name == "rx":
        qml.RX(params[0], wires=qs[0])
    elif name == "ry":
        qml.RY(params[0], wires=qs[0])
    elif name == "rz":
        qml.RZ(params[0], wires=qs[0])
    elif name == "u1":
        qml.PhaseShift(params[0], wires=qs[0])
    elif name == "u2":
        phi, lam = params[0], params[1]
        qml.U2(phi, lam, wires=qs[0])
    elif name == "u3":
        theta, phi, lam = params[0], params[1], params[2]
        qml.U3(theta, phi, lam, wires=qs[0])
    elif name in ("cx", "cnot"):
        qml.CNOT(wires=[qs[0], qs[1]])
    elif name == "cz":
        qml.CZ(wires=[qs[0], qs[1]])
    elif name == "swap":
        qml.SWAP(wires=[qs[0], qs[1]])
    elif name == "ccx":
        qml.Toffoli(wires=[qs[0], qs[1], qs[2]])
    else:
        raise NotImplementedError(
            f"Unsupported gate for PennyLane bridge: {op.name}"
        )


def _qiskit_to_qfunc_for_compile(qc: QuantumCircuit) -> Callable:
    """Build a PennyLane quantum function from a Qiskit circuit (no measurements)."""

    def qfunc():
        for inst in qc.data:
            _apply_qiskit_instruction_to_qml(inst, qc)

    return qfunc


def _run_qiskit_benchmark(qasm_path: str) -> BenchResult:
    r = BenchResult(backend="qiskit", qasm_path=qasm_path)
    tracemalloc.start()
    t0 = time.perf_counter()
    try:
        qc = QuantumCircuit.from_qasm_file(qasm_path)
        tqc = transpile(
            qc,
            basis_gates=BASIS_GATES,
            optimization_level=QISKIT_OPTIMIZATION_LEVEL,
        )
        qasm_str = qasm2_dumps(tqc)
        gates, depth = qiskit_metrics_from_qasm(qasm_str)
        r.gate_count, r.depth = gates, depth
        r.notes = (
            f"basis={BASIS_GATES}, optimization_level={QISKIT_OPTIMIZATION_LEVEL}"
        )
    except Exception as e:
        r.error = str(e)
    finally:
        r.compile_time_s = time.perf_counter() - t0
        _, peak = tracemalloc.get_traced_memory()
        tracemalloc.stop()
        r.memory_peak_bytes = peak
    return r


def _run_cirq_benchmark(qasm_path: str) -> BenchResult:
    r = BenchResult(backend="cirq", qasm_path=qasm_path)
    if not _CIRQ_AVAILABLE:
        r.error = "cirq not installed"
        return r

    tracemalloc.start()
    t0 = time.perf_counter()
    try:
        with warnings.catch_warnings():
            warnings.simplefilter("ignore", category=FutureWarning)
            with open(qasm_path, encoding="utf-8", errors="replace") as f:
                qasm_in = f.read()
            # Cirq's QASM 2 parser does not support barrier
            qasm_in = re.sub(
                r"^\s*barrier[^;]*;\s*", "", qasm_in, flags=re.MULTILINE
            )
            circuit = circuit_from_qasm(qasm_in)
            opt = cirq.optimize_for_target_gateset(
                circuit, gateset=cirq.CZTargetGateset()
            )
            qasm_out = cirq.qasm(opt)
        gates, depth = qiskit_metrics_from_qasm(qasm_out)
        r.gate_count, r.depth = gates, depth
        r.notes = "Cirq CZTargetGateset (PhasedXZ + CZ); metrics via Qiskit QASM parse"
    except Exception as e:
        r.error = str(e)
    finally:
        r.compile_time_s = time.perf_counter() - t0
        _, peak = tracemalloc.get_traced_memory()
        tracemalloc.stop()
        r.memory_peak_bytes = peak
    return r


def _run_pennylane_benchmark(qasm_path: str) -> BenchResult:
    r = BenchResult(backend="pennylane", qasm_path=qasm_path)
    if not _PL_AVAILABLE:
        r.error = "pennylane not installed"
        return r

    tracemalloc.start()
    t0 = time.perf_counter()
    try:
        qc = QuantumCircuit.from_qasm_file(qasm_path)
        qc = qc.remove_final_measurements(inplace=False)
        n = qc.num_qubits
        qfunc = _qiskit_to_qfunc_for_compile(qc)
        fn_compiled = qml.compile(
            qfunc,
            basis_set=["RZ", "RX", "RY", "H", "CNOT"],
            num_passes=2,
        )
        dev = qml.device("default.qubit", wires=n)
        qnode = qml.QNode(fn_compiled, dev)
        qasm_out = qml.to_openqasm(qnode, measure_all=False, rotations=False)()
        gates, depth = qiskit_metrics_from_qasm(qasm_out)
        r.gate_count, r.depth = gates, depth
        r.notes = (
            "PennyLane qml.compile (RZ,RX,RY,H,CNOT), input from Qiskit circuit "
            "(no pennylane-qiskit); metrics via Qiskit QASM parse"
        )
    except Exception as e:
        r.error = str(e)
    finally:
        r.compile_time_s = time.perf_counter() - t0
        _, peak = tracemalloc.get_traced_memory()
        tracemalloc.stop()
        r.memory_peak_bytes = peak
    return r


def _subprocess_peak_rss(
    args: list[str],
    cwd: Optional[str] = None,
    poll_interval_s: float = 0.02,
) -> tuple[int, subprocess.CompletedProcess[str]]:
    """Run subprocess and return (peak_rss_bytes, completed_process)."""
    import psutil

    proc = subprocess.Popen(
        args,
        cwd=cwd,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )
    peak = 0
    done = threading.Event()

    def _poll():
        nonlocal peak
        try:
            p = psutil.Process(proc.pid)
            while not done.is_set():
                try:
                    peak = max(peak, p.memory_info().rss)
                    for ch in p.children(recursive=True):
                        try:
                            peak = max(peak, ch.memory_info().rss)
                        except (psutil.NoSuchProcess, psutil.AccessDenied):
                            pass
                except (psutil.NoSuchProcess, psutil.AccessDenied):
                    break
                time.sleep(poll_interval_s)
        except Exception:
            pass

    t = threading.Thread(target=_poll, daemon=True)
    t.start()
    out, err = proc.communicate()
    done.set()
    t.join(timeout=2.0)
    cp = subprocess.CompletedProcess(args, proc.returncode, out, err)
    return peak, cp


def _run_qllvm_benchmark(
    qasm_path: str,
    qllvm_bin: str = "qllvm",
    out_dir: Optional[str] = None,
) -> BenchResult:
    """
    QLLVM CLI: same flags as Performance_testing_CN.ipynb.
    Peak memory = max RSS sampled for child process tree (approximate).
    """
    r = BenchResult(backend="qllvm", qasm_path=qasm_path)
    algorithm_name = os.path.splitext(os.path.basename(qasm_path))[0]
    if out_dir is None:
        out_dir = tempfile.mkdtemp(prefix="qllvm_bench_")
        cleanup = True
    else:
        os.makedirs(out_dir, exist_ok=True)
        cleanup = False

    out_path = os.path.join(out_dir, algorithm_name)
    args = [
        qllvm_bin,
        qasm_path,
        "-qrt",
        "nisq",
        "-qpu",
        "qasm-backend",
        "-o",
        out_path,
        "-O1",
        "-basicgate=[rx,ry,rz,cz,h]",
    ]
    if not shutil.which(qllvm_bin) and os.path.isfile(qllvm_bin):
        pass  # use absolute path

    t0 = time.perf_counter()
    try:
        peak_rss, cp = _subprocess_peak_rss(args)
        r.memory_peak_bytes = peak_rss
        r.compile_time_s = time.perf_counter() - t0
        if cp.returncode != 0:
            r.error = (cp.stderr or cp.stdout or "")[:2000]
            return r
        qasm_file = out_path + ".qasm"
        if not os.path.isfile(qasm_file):
            r.error = f"missing output {qasm_file}"
            return r
        r.gate_count, r.depth = qiskit_metrics_from_file(qasm_file)
        r.notes = "QLLVM -O1 basicgate rx,ry,rz,cz,h; metrics via Qiskit QASM parse"
    except Exception as e:
        r.error = str(e)
        r.compile_time_s = time.perf_counter() - t0
    finally:
        if cleanup:
            try:
                shutil.rmtree(out_dir, ignore_errors=True)
            except Exception:
                pass
    return r


def benchmark_one_file(
    qasm_path: str,
    *,
    backends: Optional[list[str]] = None,
    qllvm_bin: str = "qllvm",
    qllvm_out_dir: Optional[str] = None,
) -> list[BenchResult]:
    """
    Run selected backends on one OpenQASM file.

    backends: subset of ["qllvm", "qiskit", "cirq", "pennylane"]. Default: all.
    """
    if backends is None:
        backends = ["qllvm", "qiskit", "cirq", "pennylane"]

    runners: dict[str, Callable[[], BenchResult]] = {
        "qiskit": lambda: _run_qiskit_benchmark(qasm_path),
        "cirq": lambda: _run_cirq_benchmark(qasm_path),
        "pennylane": lambda: _run_pennylane_benchmark(qasm_path),
        "qllvm": lambda: _run_qllvm_benchmark(
            qasm_path, qllvm_bin=qllvm_bin, out_dir=qllvm_out_dir
        ),
    }

    results: list[BenchResult] = []
    for name in backends:
        if name not in runners:
            results.append(
                BenchResult(
                    backend=name,
                    qasm_path=qasm_path,
                    error=f"unknown backend {name}",
                )
            )
            continue
        results.append(runners[name]())
    return results


def results_to_rows(results: list[BenchResult]) -> list[dict[str, Any]]:
    """Flatten for pandas.DataFrame."""
    rows = []
    for x in results:
        rows.append(
            {
                "backend": x.backend,
                "qasm_path": x.qasm_path,
                "gate_count": x.gate_count,
                "depth": x.depth,
                "compile_time_s": x.compile_time_s,
                "memory_peak_bytes": x.memory_peak_bytes,
                "memory_peak_MiB": (x.memory_peak_bytes / (1024 * 1024))
                if x.memory_peak_bytes is not None
                else None,
                "error": x.error,
                "notes": x.notes,
            }
        )
    return rows
