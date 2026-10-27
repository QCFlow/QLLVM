"""QLLVM hooks for the unchanged official Benchpress workouts."""
import os
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "upstream/benchpress"))


def pytest_addoption(parser):
    group = parser.getgroup("qllvm")
    group.addoption("--qllvm-compiler", default=str(ROOT.parent.parent / "build/bin/qllvm-compile"))
    group.addoption("--qllvm-timeout", type=float, default=60)
    group.addoption("--qllvm-max-qubits", type=int, default=0,
                    help="0 means no width filter; skipped resource limits remain in native reports")


def pytest_configure(config):
    from benchpress.config import Configuration
    # Only the neutral Qiskit circuit representation, I/O metrics and backend
    # validator are used. Compilation always invokes QLLVM, never Qiskit passes.
    Configuration.gym_name = "qiskit"


def pytest_benchmark_update_json(config, benchmarks, output_json):
    import hashlib
    import json
    path = Path(config.getoption("--qllvm-compiler")).resolve()
    output_json["qllvm"] = {"compiler": str(path), "compiler_sha256": hashlib.sha256(path.read_bytes()).hexdigest(),
                            "snapshot": json.loads((ROOT / "benchpress.source.json").read_text())["revision"],
                            "scope": "Official Benchpress workouts; native target gate-set/topology validation"}


def pytest_report_header(config):
    return "QLLVM official Benchpress adapter; output metrics/validation use Qiskit circuit I/O only"
