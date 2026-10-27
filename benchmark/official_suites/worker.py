#!/usr/bin/env python3
"""Run a single framework in a bounded worker process. Never simulate hardware."""
import argparse
import importlib.metadata
import json
from pathlib import Path
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parent
BASIS = ["rx", "ry", "rz", "cz", "h", "x", "y", "z"]


def load_qasm(path):
    from qiskit import qasm2, qasm3
    if "OPENQASM 3" in path.read_text(errors="replace")[:4096]:
        return qasm3.load(str(path))
    return qasm2.load(str(path), custom_instructions=qasm2.LEGACY_CUSTOM_INSTRUCTIONS)


def gate_metrics(circuit):
    gates = [x for x in circuit.data if x.operation.name not in ("measure", "barrier")]
    return {"qubits": circuit.num_qubits, "classical_bits": circuit.num_clbits,
            "gates": len(gates), "two_qubit_gates": sum(x.operation.num_qubits == 2 for x in gates),
            "depth": circuit.depth(), "operations": dict(circuit.count_ops())}


def prepare(path, output):
    from qiskit import qasm2, transpile
    circuit = load_qasm(path)
    # Universal, target-independent serialization only, shared by every compiler.
    lowered = transpile(circuit, basis_gates=["u3", "cx"], optimization_level=0,
                        seed_transpiler=12345)
    output.write_text(qasm2.dumps(lowered) + "\n")
    from verification import verify
    check = verify(circuit, load_qasm(output), 6, 1e-7, "coherent")
    if check["status"] == "failed":
        raise RuntimeError("Shared serialization failed semantic check: " + str(check))
    return gate_metrics(lowered)


def pennylane_compile(path, output):
    import pennylane as qml
    from qiskit import qasm2
    from qiskit.circuit import QuantumCircuit
    # Only u3/cx/measure/barrier inputs enter this bridge. Anything else is an
    # explicit unsupported result. Measurements are restored with exact wiring.
    circuit = load_qasm(path)
    ops, mapping = [], []
    measured = set()
    for inst in circuit.data:
        name = inst.operation.name
        qs = [circuit.find_bit(q).index for q in inst.qubits]
        if name == "barrier":
            continue
        if name == "measure":
            c = circuit.find_bit(inst.clbits[0]).index
            if qs[0] in measured or any(c == old_c for _, old_c in mapping):
                raise NotImplementedError("Repeated measurements require instrument-level support")
            measured.add(qs[0]); mapping.append((qs[0], c)); continue
        if measured.intersection(qs):
            raise NotImplementedError("PennyLane bridge does not remove intermediate measurements")
        if getattr(inst.operation, "condition", None) is not None:
            raise NotImplementedError("Classical feedback is not supported by this bridge")
        if name == "u3":
            ops.append(qml.U3(*map(float, inst.operation.params), wires=qs[0]))
        elif name == "cx":
            ops.extend([qml.Hadamard(wires=qs[1]), qml.CZ(wires=qs),
                        qml.Hadamard(wires=qs[1])])
        else:
            raise NotImplementedError("PennyLane bridge requires canonical u3/cx input: " + name)
    tape = qml.tape.QuantumScript(ops, [])
    # Native PennyLane compiler, not Qiskit optimization disguised as PennyLane.
    tapes, _ = qml.transforms.compile(tape, basis_set=["RX", "RY", "RZ", "CZ",
                        "Hadamard", "PauliX", "PauliY", "PauliZ"], num_passes=2)
    compiled = tapes[0]
    result = QuantumCircuit(circuit.num_qubits, circuit.num_clbits)
    functions = {"RX": result.rx, "RY": result.ry, "RZ": result.rz,
                 "Hadamard": result.h, "PauliX": result.x, "PauliY": result.y,
                 "PauliZ": result.z, "CZ": result.cz}
    for op in compiled.operations:
        if op.name not in functions:
            raise NotImplementedError("PennyLane export gate: " + op.name)
        functions[op.name](*map(float, op.parameters), *list(op.wires))
    for q, c in mapping:
        result.measure(q, c)
    output.write_text(qasm2.dumps(result) + "\n")


def pyqpanda_compile(path, output):
    import pyqpanda as pq
    # QASM import/export and native optimizer are supplied by pyqpanda. No
    # quantum machine execution or state vector allocation is requested.
    machine = pq.CPUQVM()
    machine.init_qvm()
    try:
        program, qubits, cbits = pq.convert_qasm_string_to_qprog(path.read_text(), machine)
        optimized = pq.circuit_optimizer(program, mode_list=[pq.QCircuitOPtimizerMode.Merge_H_X,
                            pq.QCircuitOPtimizerMode.Merge_RX, pq.QCircuitOPtimizerMode.Merge_RY,
                            pq.QCircuitOPtimizerMode.Merge_RZ])
        optimized = pq.transform_to_base_qgate(optimized, machine,
                        ["RX", "RY", "RZ", "H", "X", "Y", "Z"], ["CZ"])
        text = pq.convert_qprog_to_qasm(optimized, machine)
        output.write_text(text + "\n")
    finally:
        machine.finalize()


def pyqpanda3_compile(path, output):
    from pyqpanda3.intermediate_compiler import convert_qasm_string_to_qprog, convert_qprog_to_qasm
    from pyqpanda3.transpilation import Transpiler, TranspilationExecutionConfig
    program = convert_qasm_string_to_qprog(path.read_text())
    n = len(program.qubits())
    config = TranspilationExecutionConfig()
    config.seed = 12345
    config.max_threads = 1
    topology = [[i, j] for i in range(n) for j in range(n) if i != j]
    mapping = {i:i for i in range(n)} if n > 1 else {}
    result = Transpiler().transpile(program, topology, mapping, 2,
                       ["RX", "RY", "RZ", "CZ", "H", "X", "Y", "Z"], config)
    output.write_text(convert_qprog_to_qasm(result, 16) + "\n")


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--request", type=Path, required=True)
    a = json.loads(p.parse_args().request.read_text())
    path, directory = Path(a["input"]), Path(a["directory"])
    output = directory / "compiled.qasm"
    result = {"framework": a["framework"], "status": "failed"}
    # Import framework before timing. The reported time covers the complete
    # parse/compile/export pipeline; startup and verification are separate.
    try:
        framework = a["framework"]
        if framework == "prepare":
            result["input_metrics"] = prepare(path, output)
            result["status"] = "prepared"
        else:
            if framework == "qiskit":
                import qiskit
                from qiskit import qasm2, transpile
                result["version"] = importlib.metadata.version("qiskit")
            elif framework == "tket":
                from pytket.qasm import circuit_from_qasm, circuit_to_qasm
                from pytket.passes import FullPeepholeOptimise, AutoRebase, SequencePass
                from pytket.circuit import OpType
                result["version"] = importlib.metadata.version("pytket")
            elif framework == "pyqpanda":
                import pyqpanda
                result["version"] = importlib.metadata.version("pyqpanda")
            elif framework == "pennylane":
                import pennylane
                result["version"] = importlib.metadata.version("pennylane")
            elif framework == "pyqpanda3":
                import pyqpanda3
                from pyqpanda3 import intermediate_compiler, transpilation
                result["version"] = importlib.metadata.version("pyqpanda3")
            elif framework != "qllvm":
                raise ValueError("Unknown framework")
            expected = a.get("expected_version")
            if expected and result.get("version") != expected:
                raise RuntimeError(f"Expected pinned latest version {expected}, got {result.get('version')}")
            # All workers use this common I/O library for input/output metrics;
            # module import time must not enter only one framework's pipeline.
            from qiskit import qasm2 as common_qasm2, qasm3 as common_qasm3
            start = time.perf_counter()
            if framework == "qllvm":
                command = [a["compiler"], str(path), "-" + a["opt"],
                           "-emit-backend=qasm-backend", "-output-path=" + str(output),
                           "-basicgate=[" + ",".join(BASIS) + "]"]
                result["command"] = command
                with (directory / "compiler.log").open("w") as log:
                    done = subprocess.run(command, cwd=directory, stdout=log, stderr=subprocess.STDOUT)
                if done.returncode:
                    raise RuntimeError(f"Compiler exit {done.returncode}; see compiler.log")
            elif framework == "qiskit":
                circuit = load_qasm(path)
                circuit = transpile(circuit, basis_gates=BASIS, optimization_level=3, seed_transpiler=12345)
                output.write_text(qasm2.dumps(circuit) + "\n")
            elif framework == "tket":
                circuit = circuit_from_qasm(str(path))
                # QASM preserves explicit wire identities. Disallow implicit
                # swaps so final permutations cannot move through measurements.
                FullPeepholeOptimise(allow_swaps=False).apply(circuit)
                AutoRebase({OpType.Rx, OpType.Ry, OpType.Rz,
                            OpType.CZ, OpType.H, OpType.X, OpType.Y, OpType.Z}).apply(circuit)
                circuit_to_qasm(circuit, str(output), maxwidth=max(circuit.n_qubits, circuit.n_bits, 32))
            elif framework == "pyqpanda":
                pyqpanda_compile(path, output)
            elif framework == "pyqpanda3":
                pyqpanda3_compile(path, output)
            else:
                pennylane_compile(path, output)
            result["pipeline_seconds"] = time.perf_counter() - start
            if not output.is_file() or output.stat().st_size == 0:
                raise RuntimeError("No compiler output")
            result["status"] = "compiled"
            try:
                compiled = load_qasm(output)
                original = load_qasm(path)
                result["input_metrics"] = gate_metrics(original)
                result["output_metrics"] = gate_metrics(compiled)
                # Same operator checker for every framework; contracts are never
                # weakened in response to an optimization failure.
                from verification import verify
                result["verification"] = verify(original, compiled, a["verify_qubits"],
                                                  a["tolerance"], "coherent")
                status = result["verification"]["status"]
                result["status"] = {"passed": "verified", "not_checked": "compile_only",
                        "unsupported": "compiled_unverified", "failed": "semantic_failed"}[status]
                unexpected = set(compiled.count_ops()) - set(BASIS) - {"measure", "barrier"}
                result["common_basis"] = not unexpected
                if unexpected:
                    result["basis_note"] = "Native output has gates outside common basis: " + repr(sorted(unexpected))
            except Exception as exc:
                result["status"] = "compiled_unverified"
                result["verification"] = {"status": "unsupported", "reason": str(exc)}
    except ImportError as exc:
        result.update(status="unavailable", error=str(exc))
    except NotImplementedError as exc:
        result.update(status="unsupported", error=str(exc))
    except Exception as exc:
        result.update(error=f"{type(exc).__name__}: {exc}")
    (directory / "worker.json").write_text(json.dumps(result, indent=2) + "\n")
    print(json.dumps(result))


if __name__ == "__main__":
    main()
