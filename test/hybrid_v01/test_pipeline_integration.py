"""Keep the reviewed direct-input optimization contract after pass integration."""
import json
import subprocess

from qiskit import QuantumCircuit, qasm2
from qiskit.quantum_info import Operator

import qllvm


def test_direct_input_does_not_enable_unvalidated_phase_or_linear_passes(tmp_path, compiler):
    reference = QuantumCircuit(3)
    reference.h(0)
    reference.cx(0, 1)
    reference.rz(5e-7, 1)
    reference.cx(0, 1)
    reference.cx(1, 2)
    reference.cx(0, 1)
    source, output = tmp_path/'program.json', tmp_path/'output.qasm'
    source.write_text(json.dumps(qllvm.import_program(reference).bind().to_dict()))
    result = subprocess.run([compiler,str(source),'-input-backend=program-json',
        '-no-entrypoint','-internal-func-name=direct','-O1','-trace-timings',
        '-mlir-disable-threading','-qpu=qasm-backend',
        '-emit-backend=qasm-backend','-output-path='+str(output)],
        capture_output=True,text=True,timeout=60)
    assert result.returncode == 0, result.stdout+result.stderr
    assert 'direct-conservative-optimization' in result.stderr
    assert 'PhaseGadgetRemoval' not in result.stderr
    assert 'LinearReversible' not in result.stderr
    compiled=qasm2.loads(output.read_text(),custom_instructions=qasm2.LEGACY_CUSTOM_INSTRUCTIONS)
    assert Operator(reference).equiv(Operator(compiled),atol=1e-9,rtol=0)
