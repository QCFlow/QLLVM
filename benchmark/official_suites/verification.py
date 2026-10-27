"""Shared semantic checks for optional official benchmark suites."""
import math

def terminal_unitary(circuit):
    """Return the unitary body and terminal quantum-to-classical measurement map.

    Resets, feedback, intermediate measurements and other nonunitary operations
    are rejected rather than silently removed. Register names are immaterial,
    while bit indices and measurement destinations remain part of the check.
    """
    body = circuit.copy_empty_like()
    measured = set()
    mapping = []
    for inst in circuit.data:
        op = inst.operation
        qubits = [circuit.find_bit(q).index for q in inst.qubits]
        cbits = [circuit.find_bit(c).index for c in inst.clbits]
        if getattr(op, "condition", None) is not None or op.name in ("reset", "if_else", "while_loop", "for_loop"):
            raise ValueError("Dynamic/nonunitary circuit requires an instrument-level checker")
        if op.name == "barrier":
            continue
        if op.name == "measure":
            mapping.append((qubits[0], cbits[0]))
            measured.add(qubits[0])
        else:
            if measured.intersection(qubits):
                raise ValueError("Non-terminal measurement requires an instrument-level checker")
            if cbits:
                raise ValueError("Classical operation cannot be checked as a unitary")
            body.append(op, qubits)
    body.global_phase = circuit.global_phase
    # Repeated measurements or repeated classical destinations need instrument
    # semantics; do not weaken this into a set comparison.
    if len({q for q, _ in mapping}) != len(mapping) or len({c for _, c in mapping}) != len(mapping):
        raise ValueError("Repeated measurement requires an instrument-level checker")
    return body, sorted(mapping)


def verify(original, compiled, max_qubits, tolerance):
    """Check full operators (global phase allowed), including measurement wiring."""
    import numpy as np
    from qiskit.quantum_info import Operator

    if original.num_qubits != compiled.num_qubits:
        return {"status": "failed", "reason": "Qubit count changed"}
    if original.num_clbits != compiled.num_clbits:
        return {"status": "failed", "reason": "Classical bit count changed"}
    try:
        left, left_map = terminal_unitary(original)
        right, right_map = terminal_unitary(compiled)
    except ValueError as exc:
        return {"status": "unsupported", "reason": str(exc)}
    if left_map != right_map:
        return {"status": "failed", "reason": "Terminal measurement mapping changed", "input_mapping": left_map, "output_mapping": right_map}
    if original.num_qubits > max_qubits:
        return {"status": "not_checked", "reason": "Above full-operator qubit limit"}
    try:
        a, b = Operator(left).data, Operator(right).data
    except Exception as exc:
        return {"status": "unsupported", "reason": "Operator construction: " + str(exc)}
    index = np.unravel_index(np.argmax(np.abs(a)), a.shape)
    ratio = b[index] / a[index]
    if abs(ratio) == 0:
        return {"status": "failed", "reason": "Zero amplitude at phase reference"}
    phase = ratio / abs(ratio)
    error = float(np.max(np.abs(b - phase * a)))
    return {
        "status": "passed" if math.isfinite(error) and error <= tolerance else "failed",
        "method": "full_operator_up_to_global_phase_and_terminal_measurement_map",
        "max_abs_error": error, "tolerance": tolerance,
    }


_operator_verify = verify

def verify_terminal_classical(original, compiled, max_qubits, tolerance):
    import numpy as np
    from qiskit.quantum_info import Operator
    if original.num_qubits != compiled.num_qubits or original.num_clbits != compiled.num_clbits:
        return {"status": "failed", "reason": "Quantum/classical width changed"}
    if original.num_qubits > max_qubits:
        return {"status": "not_checked", "reason": "Above POVM checker qubit limit"}
    try:
        left, lm = terminal_unitary(original)
        right, rm = terminal_unitary(compiled)
        if not lm or not rm:
            return {"status": "unsupported", "reason": "Classical-output mode requires explicit measurements"}
        a, b = Operator(left).data, Operator(right).data
        def groups(mapping, n):
            result = {}
            for row in range(1 << n):
                outcome = sum(((row >> q) & 1) << c for q,c in mapping)
                result.setdefault(outcome, []).append(row)
            return result
        ga,gb = groups(lm,original.num_qubits),groups(rm,compiled.num_qubits)
        error = 0.0
        for outcome in set(ga) | set(gb):
            va = a[ga.get(outcome,[])]
            vb = b[gb.get(outcome,[])]
            ea,eb = va.conj().T @ va, vb.conj().T @ vb
            error = max(error,float(np.max(np.abs(ea-eb))))
        return {"status": "passed" if math.isfinite(error) and error<=tolerance else "failed",
                "method": "terminal_classical_output_POVM_all_input_states",
                "max_abs_effect_error": error, "tolerance": tolerance,
                "scope": "Only declared terminal classical output; no coherent output or controlled embedding claim"}
    except Exception as exc:
        return {"status": "unsupported", "reason": str(exc)}

def verify(original, compiled, max_qubits, tolerance, semantics="coherent"):
    if semantics not in ("coherent", "terminal-classical"):
        raise ValueError("Unknown semantic contract: " + semantics)
    operator = _operator_verify(original,compiled,max_qubits,tolerance)
    if semantics == "coherent":
        return operator
    classical = verify_terminal_classical(original,compiled,max_qubits,tolerance)
    classical["coherent_operator_diagnostic"] = operator
    return classical
