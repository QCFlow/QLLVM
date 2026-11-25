"""Compatibility text conversion entry; prefer import_program/build.

Copyright QCFlow 2026. Licensed under Apache-2.0.
"""
import os
from pathlib import Path
import subprocess
import tempfile


def compile(circuit, input, out_backend="qasm-backend", backend_config="", basicgate="",
            optlevel="0", output_path="", state=False, error_print=False):
    if input in ("qasm", "qcis"):
        source = circuit
    elif input == "qiskit":
        from qiskit.qasm2 import dumps
        source = dumps(circuit)
    elif input == "cirq":
        import cirq
        source = cirq.qasm(circuit)
    elif input in ("qprog", "originir"):
        from pyqpanda3.intermediate_compiler import convert_qprog_to_qasm, convert_originir_string_to_qprog
        source = convert_qprog_to_qasm(circuit if input == "qprog" else convert_originir_string_to_qprog(circuit))
    else:
        raise ValueError(f"Input frontend {input} not supported")
    if not isinstance(source, str) or not source.strip():
        raise ValueError("The circuit to be compiled is empty")
    if str(optlevel) not in ("0", "1"):
        raise ValueError("optlevel must be 0 or 1")
    with tempfile.TemporaryDirectory(prefix="qllvm-legacy-") as temporary:
        origin = Path(temporary) / "program.qasm"
        origin.write_text(source)
        output = os.path.splitext(output_path)[0] if output_path else str(Path.cwd() / "_temp_compiled")
        command = ["qllvm", str(origin), "-qrt", "nisq", "-qpu", out_backend, f"-O{optlevel}", "-o", output]
        if backend_config:
            command += ["-qpu-config", str(backend_config)]
        if basicgate:
            command += [f"-basicgate={basicgate}"]
        if state:
            command.append("-circuit-state")
        if error_print:
            command.append("-fatal-error-verbose")
        subprocess.run(command, check=True)
