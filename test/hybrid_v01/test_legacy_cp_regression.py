"""CP block synthesis must preserve its phase for every quantum input."""
import math
from pathlib import Path
import subprocess

import pytest
from qiskit import QuantumCircuit, qasm2
from qiskit.quantum_info import Operator, Statevector


@pytest.mark.parametrize("theta", [0.0, 0.1, 0.37, -0.37, 1.0, math.pi / 2, math.pi])
@pytest.mark.parametrize("direction", [(0, 1), (1, 0)])
@pytest.mark.parametrize("schedule", ["default", "block_only"])
def test_legacy_cp_block_phase(tmp_path, compiler, theta, direction, schedule):
    source = QuantumCircuit(2)
    source.ry(0.9, direction[0])
    source.cx(*direction)
    source.rz(0.3894183423086505, direction[1])
    source.cp(theta, *direction)
    inp, out = tmp_path / "phase.qasm", tmp_path / "phase-out.qasm"
    inp.write_text(qasm2.dumps(source))
    command = [compiler, str(inp), "-O1", "-exact-rotations", "-mlir-disable-threading",
               "-qpu=qasm-backend", "-emit-backend=qasm-backend", f"-output-path={out}"]
    if schedule == "block_only":
        passes = tmp_path / "passes.txt"
        passes.write_text("1\n8\n")
        command.append(f"-customPassSequence={passes}")
    result = subprocess.run(command, capture_output=True, text=True, timeout=90)
    assert result.returncode == 0, result.stderr + result.stdout
    compiled = qasm2.loads(out.read_text(), custom_instructions=qasm2.LEGACY_CUSTOM_INSTRUCTIONS)
    assert Operator(source).equiv(Operator(compiled), atol=1e-8, rtol=0)
    # Interference exposes phase errors that terminal Z-basis counts can hide.
    for circuit in (source, compiled):
        circuit.h(0)
        circuit.h(1)
    assert Statevector.from_instruction(source).equiv(Statevector.from_instruction(compiled), atol=1e-8, rtol=0)
