"""Implement official Benchpress compilation workouts for the QLLVM CLI.

Circuit construction and manipulation retain the official default SKIPPED status:
QLLVM's CLI is a compiler, with no matching in-memory circuit SDK interface.
"""
from pathlib import Path
import os
import signal
import subprocess
import tempfile

import pytest
from qiskit import qasm2

from benchpress.config import Configuration
from benchpress.utilities.backends import FlexibleBackend
from benchpress.utilities.io import output_circuit_properties, qasm_circuit_loader
from benchpress.utilities.io.hamiltonians import generate_hamiltonian_circuit
from benchpress.qiskit_gym.utils.validation import qiskit_circuit_validation
from benchpress.workouts.abstract_transpile import (
    WorkoutAbstractQasmBenchSmall, WorkoutAbstractQasmBenchMedium,
    WorkoutAbstractQasmBenchLarge,
)
from benchpress.workouts.abstract_transpile.qasmbench import (
    SMALL_CIRC_TOPO, SMALL_NAMES, MEDIUM_CIRC_TOPO, MEDIUM_NAMES,
    LARGE_CIRC_TOPO, LARGE_NAMES,
)
from benchpress.workouts.abstract_transpile.hamlib_hamiltonians import (
    WorkoutAbstractHamiltonians, HAM_TOPO, HAM_TOPO_NAMES,
)
from benchpress.workouts.build import WorkoutCircuitConstruction
from benchpress.workouts.manipulate import WorkoutCircuitManipulate
from benchpress.workouts.device_transpile import (
    WorkoutDeviceFeynman, WorkoutDeviceHamlibHamiltonians, WorkoutDeviceTranspile100Q,
)
from benchpress.workouts.validation import benchpress_test_validation


def compile_workout(circuit, backend, benchmark, request):
    maximum = request.config.getoption("--qllvm-max-qubits")
    if maximum and circuit.num_qubits > maximum:
        pytest.skip(f"Explicit resource limit: {circuit.num_qubits} > {maximum} qubits")
    if circuit.num_qubits > backend.num_qubits:
        pytest.skip("Official backend has too few qubits")
    basis = [g for g in backend.operation_names if g in ("id", "sx", "x", "rz", "cz", "cx", "rx", "ry", "h")]
    edges = backend.coupling_map.get_edges()
    with tempfile.TemporaryDirectory(prefix="qllvm-benchpress-") as temporary:
        directory = Path(temporary)
        source, output = directory / "input.qasm", directory / "compiled.qasm"
        source.write_text(qasm2.dumps(circuit) + "\n")
        command = [str(Path(request.config.getoption("--qllvm-compiler")).resolve()),
                   str(source), "-O1", "-emit-backend=qasm-backend", "-output-path=" + str(output),
                   "-basicgate=[" + ",".join(basis) + "]"]
        if edges and backend.coupling_map.size() > 1:
            command += ["-sabre-coupling-map=" + ";".join(f"{a},{b}" for a,b in edges)]
        def compile_native():
            if output.exists():
                output.unlink()
            with (directory / "compiler.log").open("w") as log:
                process = subprocess.Popen(command, stdout=log, stderr=subprocess.STDOUT, start_new_session=True)
                try:
                    process.wait(timeout=request.config.getoption("--qllvm-timeout"))
                except subprocess.TimeoutExpired:
                    os.killpg(process.pid, signal.SIGKILL); process.wait()
                    pytest.fail("QLLVM compilation timed out")
            assert process.returncode == 0, (directory / "compiler.log").read_text()[-4000:]
            assert output.is_file() and output.stat().st_size, "QLLVM returned no output"
            return output.read_text()
        text = benchmark(compile_native)
        result = qasm2.loads(text, custom_instructions=qasm2.LEGACY_CUSTOM_INSTRUCTIONS)
        output_circuit_properties(result, backend.two_q_gate_type, benchmark)
        benchmark.extra_info["qllvm_command"] = command
        benchmark.extra_info["validation_scope"] = "Official gate-set and topology check; no implicit semantic-equivalence claim"
        assert qiskit_circuit_validation(result, backend)


def qasm_workout(circ_and_topo, benchmark, request):
    circuit = qasm_circuit_loader(circ_and_topo[0], benchmark)
    backend = FlexibleBackend(circuit.num_qubits, circ_and_topo[1], control_flow=True)
    compile_workout(circuit, backend, benchmark, request)


@benchpress_test_validation
class TestWorkoutAbstractQasmBenchSmall(WorkoutAbstractQasmBenchSmall):
    @pytest.mark.parametrize("circ_and_topo", SMALL_CIRC_TOPO, ids=SMALL_NAMES)
    def test_QASMBench_small(self, benchmark, circ_and_topo, request):
        qasm_workout(circ_and_topo, benchmark, request)


@benchpress_test_validation
class TestWorkoutAbstractQasmBenchMedium(WorkoutAbstractQasmBenchMedium):
    @pytest.mark.parametrize("circ_and_topo", MEDIUM_CIRC_TOPO, ids=MEDIUM_NAMES)
    def test_QASMBench_medium(self, benchmark, circ_and_topo, request):
        qasm_workout(circ_and_topo, benchmark, request)


@benchpress_test_validation
class TestWorkoutAbstractQasmBenchLarge(WorkoutAbstractQasmBenchLarge):
    @pytest.mark.parametrize("circ_and_topo", LARGE_CIRC_TOPO, ids=LARGE_NAMES)
    def test_QASMBench_large(self, benchmark, circ_and_topo, request):
        qasm_workout(circ_and_topo, benchmark, request)


@benchpress_test_validation
class TestWorkoutAbstractHamiltonians(WorkoutAbstractHamiltonians):
    @pytest.mark.parametrize("circ_and_topo", HAM_TOPO, ids=HAM_TOPO_NAMES)
    def test_hamiltonians(self, benchmark, circ_and_topo, request):
        ham, topology = circ_and_topo
        maximum = request.config.getoption("--qllvm-max-qubits")
        if maximum and ham["ham_qubits"] > maximum:
            pytest.skip("Explicit Hamiltonian width resource limit")
        circuit = generate_hamiltonian_circuit(ham["ham_hamlib_hamiltonian"], benchmark)
        backend = FlexibleBackend(circuit.num_qubits, topology, control_flow=True)
        compile_workout(circuit, backend, benchmark, request)


@benchpress_test_validation
class TestWorkoutDeviceFeynman(WorkoutDeviceFeynman):
    @pytest.mark.parametrize("filename", sorted(Path(Configuration.get_qasm_dir("feynman")).glob("*.qasm")), ids=lambda p: p.stem)
    def test_feynman_transpile(self, benchmark, filename, request):
        circuit = qasm_circuit_loader(str(filename), benchmark)
        compile_workout(circuit, Configuration.backend(), benchmark, request)


class TestWorkoutCircuitConstruction(WorkoutCircuitConstruction):
    pass


class TestWorkoutCircuitManipulate(WorkoutCircuitManipulate):
    pass


class NativePassManager:
    """Pass-manager shape used only to run the unchanged official device tests."""
    def __init__(self, backend, request, directory):
        self.backend, self.request, self.directory = backend, request, directory

    def run(self, circuit):
        maximum = self.request.config.getoption("--qllvm-max-qubits")
        if maximum and circuit.num_qubits > maximum:
            pytest.skip("Explicit resource limit for official device workload")
        source, output = self.directory / "input.qasm", self.directory / "compiled.qasm"
        source.write_text(qasm2.dumps(circuit) + "\n")
        if output.exists():
            output.unlink()
        basis = [g for g in self.backend.operation_names if g in ("id", "sx", "x", "rz", "cz", "cx", "rx", "ry", "h")]
        command = [str(Path(self.request.config.getoption("--qllvm-compiler")).resolve()), str(source),
                   "-O1", "-emit-backend=qasm-backend", "-output-path=" + str(output),
                   "-basicgate=[" + ",".join(basis) + "]",
                   "-sabre-coupling-map=" + ";".join(f"{a},{b}" for a,b in self.backend.coupling_map.get_edges())]
        with (self.directory / "compiler.log").open("w") as log:
            process = subprocess.Popen(command, stdout=log, stderr=subprocess.STDOUT, start_new_session=True)
            try:
                process.wait(timeout=self.request.config.getoption("--qllvm-timeout"))
            except subprocess.TimeoutExpired:
                os.killpg(process.pid, signal.SIGKILL); process.wait()
                pytest.fail("QLLVM device compilation timed out")
        assert process.returncode == 0, (self.directory / "compiler.log").read_text()[-4000:]
        assert output.is_file() and output.stat().st_size
        return qasm2.load(str(output), custom_instructions=qasm2.LEGACY_CUSTOM_INSTRUCTIONS)


# Reuse the official device workload generators, benchmark functions, parameters
# and validation. Replace only their compiler factory in memory, leaving the
# vendored files untouched. This also preserves all official 100Q workloads.
from benchpress.qiskit_gym.device_transpile import test_hamiltonians as _hamiltonians
from benchpress.qiskit_gym.device_transpile import test_summit as _summit


def pytest_generate_tests(metafunc):
    if "hamiltonian_info" in metafunc.fixturenames:
        _hamiltonians.pytest_generate_tests(metafunc)


class TestWorkoutDeviceHamlibHamiltonians(_hamiltonians.TestWorkoutDeviceHamlibHamiltonians):
    @pytest.fixture(autouse=True)
    def qllvm_compiler(self, monkeypatch, request, tmp_path):
        monkeypatch.setattr(_hamiltonians, "generate_preset_pass_manager",
                            lambda opt, backend: NativePassManager(backend, request, tmp_path))


class TestWorkoutDeviceTranspile100Q(_summit.TestWorkoutDeviceTranspile100Q):
    @pytest.fixture(autouse=True)
    def qllvm_compiler(self, monkeypatch, request, tmp_path):
        monkeypatch.setattr(_summit, "generate_preset_pass_manager",
                            lambda opt, backend: NativePassManager(backend, request, tmp_path))
