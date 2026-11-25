"""Direct SDK object adapters. No QASM export, eval, or SDK object retention.

Copyright QCFlow 2026. Licensed under Apache-2.0.
"""
import math
from .program import ProgramBuilder, QLLVMError, constant


def _symbolic(expr, symbols):
    """Read a SymPy expression tree, never parse its printed representation."""
    import sympy as sp
    if expr in symbols:
        return {"op": "param", "id": symbols[expr]}
    if not getattr(expr, "free_symbols", set()):
        try:
            return constant(float(expr))
        except (TypeError, ValueError) as error:
            raise QLLVMError("UNSUPPORTED_EXPRESSION", "expected a real expression") from error
    names = {sp.Add: "add", sp.Mul: "mul", sp.Pow: "pow", sp.sin: "sin", sp.cos: "cos",
             sp.tan: "tan", sp.exp: "exp", sp.log: "log", sp.Abs: "abs", sp.asin: "asin",
             sp.acos: "acos", sp.atan: "atan"}
    if expr.func not in names:
        raise QLLVMError("UNSUPPORTED_EXPRESSION", f"unsupported symbolic operation {expr.func}")
    return {"op": names[expr.func], "args": [_symbolic(arg, symbols) for arg in expr.args]}


def _add_phase(builder, phase):
    if phase == constant(0):
        return
    previous = builder.data["global_phase"]
    if previous["op"] == "const" and phase["op"] == "const":
        builder.data["global_phase"] = constant(previous["value"] + phase["value"])
    elif previous == constant(0):
        builder.data["global_phase"] = phase
    elif previous["op"] == "add":
        previous["args"].append(phase)
    else:
        builder.data["global_phase"] = {"op": "add", "args": [previous, phase]}


def import_qiskit(circuit, max_operations=1_000_000, max_depth=32):
    import qiskit
    import sympy as sp
    from qiskit.circuit import QuantumCircuit, Measure, Reset, Barrier, ControlFlowOp, ParameterExpression
    from qiskit.circuit.library.standard_gates import get_standard_gate_name_mapping
    if not isinstance(circuit, QuantumCircuit):
        raise QLLVMError("FRONTEND_TYPE", "expected qiskit.QuantumCircuit")
    builder = ProgramBuilder(circuit.num_qubits, circuit.num_clbits, circuit.name or "main", "qiskit", qiskit.__version__)
    params = sorted(circuit.parameters, key=lambda p: (p.name, str(p.uuid)))
    symbols = {}
    for i, p in enumerate(params):
        pid = f"p{i}"
        builder.data["parameters"].append({"id": pid, "name": p.name, "dtype": "float64"})
        symbols[p.sympify()] = pid

    def expression(value):
        return _symbolic(value.sympify(), symbols) if isinstance(value, ParameterExpression) else constant(value)

    allowed = {"id", "h", "x", "y", "z", "s", "sdg", "t", "tdg", "sx", "rx", "ry", "rz", "p", "u", "cx", "cz", "swap", "cp"}
    standards = get_standard_gate_name_mapping()
    ancestors = set()

    def walk(qc, qmap, cmap, path, depth):
        if depth > max_depth or id(qc) in ancestors:
            raise QLLVMError("DECOMPOSITION_LIMIT", "recursive or too-deep gate definition", path)
        ancestors.add(id(qc))
        for attr in ("num_vars", "num_stretches"):
            if getattr(qc, attr, 0):
                raise QLLVMError("UNSUPPORTED_CLASSICAL_STATE", attr, path)
        _add_phase(builder, expression(qc.global_phase))
        for i, inst in enumerate(qc.data):
            at = f"{path}.data[{i}]"
            op = inst.operation
            if isinstance(op, ControlFlowOp) or getattr(op, "condition", None) is not None:
                raise QLLVMError("UNSUPPORTED_CONTROL_FLOW", "v0.1 requires a static circuit", at)
            qs = [qmap[qc.find_bit(q).index] for q in inst.qubits]
            cs = [cmap[qc.find_bit(c).index] for c in inst.clbits]
            standard = standards.get(op.name)
            direct = (op.name in allowed and standard is not None and op.base_class is standard.base_class
                      and getattr(op, "ctrl_state", None) == getattr(standard, "ctrl_state", None))
            special = op.base_class in (Measure, Reset, Barrier)
            if direct or special:
                if len(builder.data["operations"]) >= max_operations:
                    raise QLLVMError("DECOMPOSITION_LIMIT", "expanded operation limit exceeded", at)
                builder.add_operations([{"name": "u3" if op.name == "u" else op.name,
                                         "qubits": qs, "clbits": cs,
                                         "params": [expression(p) for p in op.params], "source": at}])
            elif op.definition is not None:
                walk(op.definition, qs, cs, at + ".definition", depth + 1)
            else:
                raise QLLVMError("UNSUPPORTED_OPERATION", f"no supported semantics for {op.name}", at)
        ancestors.remove(id(qc))

    builder.data["registers"] = [{"name": reg.name, "bits": [circuit.find_bit(bit).index for bit in reg]}
                                  for reg in circuit.cregs]
    walk(circuit, list(range(circuit.num_qubits)), list(range(circuit.num_clbits)), "circuit", 0)
    return builder.finish()


def import_cirq(circuit, max_operations=1_000_000, max_depth=32):
    import cirq
    import sympy as sp
    if not isinstance(circuit, cirq.AbstractCircuit):
        raise QLLVMError("FRONTEND_TYPE", "expected cirq.Circuit or FrozenCircuit")
    qubits = sorted(circuit.all_qubits())
    qmap = {q: i for i, q in enumerate(qubits)}
    builder = ProgramBuilder(len(qubits), frontend="cirq", version=cirq.__version__)
    symbols = {}
    for i, name in enumerate(sorted(cirq.parameter_names(circuit))):
        symbols[sp.Symbol(name)] = f"p{i}"
        builder.data["parameters"].append({"id": f"p{i}", "name": name, "dtype": "float64"})
    key_bits = {}

    def expression(value):
        return _symbolic(sp.sympify(value), symbols)

    def emit(name, qs, params, source, cs=()):
        if len(builder.data["operations"]) >= max_operations:
            raise QLLVMError("DECOMPOSITION_LIMIT", "expanded operation limit exceeded", source)
        builder.add_operations([{"name": name, "qubits": qs, "clbits": list(cs),
                                 "params": [expression(p) for p in params], "source": source}])

    def walk(op, path, depth=0):
        if depth > max_depth:
            raise QLLVMError("DECOMPOSITION_LIMIT", "gate definition too deep", path)
        if isinstance(op, cirq.ClassicallyControlledOperation):
            raise QLLVMError("UNSUPPORTED_CONTROL_FLOW", "v0.1 requires a static circuit", path)
        if any(q.dimension != 2 for q in op.qubits):
            raise QLLVMError("UNSUPPORTED_RESOURCE", "only qubits are supported", path)
        if isinstance(op, cirq.TaggedOperation):
            raise QLLVMError("UNSUPPORTED_METADATA", "tag semantics are not declared", path)
        gate = getattr(op, "gate", None)
        qs = [qmap[q] for q in op.qubits]
        if isinstance(gate, cirq.MeasurementGate):
            key = cirq.measurement_key_name(op)
            if key in key_bits or getattr(gate, "confusion_map", {}):
                raise QLLVMError("UNSUPPORTED_MEASUREMENT", "duplicate key or confusion map", path)
            bits = list(range(builder.data["num_clbits"], builder.data["num_clbits"] + len(qs)))
            key_bits[key] = bits
            builder.data["num_clbits"] += len(qs)
            # Cirq key integers put their first measured qubit in the most significant bit.
            builder.data["registers"].append({"name": key, "bits": list(reversed(bits))})
            for i, (q, c) in enumerate(zip(qs, bits)):
                if gate.full_invert_mask()[i]:
                    emit("x", [q], [], path + ".invert_before")
                emit("measure", [q], [], path, [c])
                if gate.full_invert_mask()[i]:
                    emit("x", [q], [], path + ".invert_after")
        elif isinstance(gate, cirq.ResetChannel):
            emit("reset", qs, [], path)
        elif isinstance(gate, cirq.GlobalPhaseGate):
            try:
                phase = float(math.atan2(complex(gate.coefficient).imag, complex(gate.coefficient).real))
            except (TypeError, ValueError) as error:
                raise QLLVMError("UNSUPPORTED_EXPRESSION", "symbolic global phase", path) from error
            _add_phase(builder, constant(phase))
        elif type(gate) in (cirq.XPowGate, cirq.YPowGate, cirq.ZPowGate,
                            type(cirq.X), type(cirq.Y), type(cirq.Z)):
            exponent = sp.sympify(gate.exponent)
            angle = sp.pi * exponent
            name = "rx" if isinstance(gate, cirq.XPowGate) else "ry" if isinstance(gate, cirq.YPowGate) else "rz"
            emit(name, qs, [angle], path)
            _add_phase(builder, expression(sp.pi * exponent * (sp.sympify(gate.global_shift) + sp.Rational(1, 2))))
        elif type(gate) in (cirq.HPowGate, cirq.CXPowGate, cirq.CZPowGate, cirq.SwapPowGate) and gate.exponent == 1:
            name = {cirq.HPowGate:"h", cirq.CXPowGate:"cx", cirq.CZPowGate:"cz", cirq.SwapPowGate:"swap"}[type(gate)]
            emit(name, qs, [], path)
            _add_phase(builder, expression(sp.pi * sp.sympify(gate.global_shift)))
        elif isinstance(op, cirq.CircuitOperation):
            raise QLLVMError("UNSUPPORTED_CONTROL_FLOW", "CircuitOperation requires structured import", path)
        else:
            decomposition = cirq.decompose_once(op, default=None)
            if decomposition is None:
                raise QLLVMError("UNSUPPORTED_OPERATION", f"unsupported Cirq operation {type(gate).__name__}", path)
            for i, child in enumerate(cirq.flatten_op_tree(decomposition)):
                walk(child, f"{path}.definition[{i}]", depth + 1)

    for mi, moment in enumerate(circuit):
        for oi, op in enumerate(moment):
            walk(op, f"circuit.moments[{mi}].operations[{oi}]")
    return builder.finish()


class FrontendRegistry:
    def __init__(self):
        self._frontends = {"qiskit": import_qiskit, "cirq": import_cirq}

    def import_program(self, circuit, frontend="qiskit", **limits):
        if frontend not in self._frontends:
            raise QLLVMError("UNKNOWN_FRONTEND", frontend)
        return self._frontends[frontend](circuit, **limits)


def import_program(circuit, frontend="qiskit", **limits):
    return FrontendRegistry().import_program(circuit, frontend, **limits)
