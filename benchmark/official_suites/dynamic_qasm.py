"""Lossless bounded bridge for single-bit feedback QASM3 benchmark circuits.

Every classical bit becomes a one-bit QASM2 register, preserving flat bit order.
No measurements, resets or feedback are erased. Unsupported control flow fails.
"""
import math


def single_bit_condition(condition, circuit):
    from qiskit.circuit import Clbit, ClassicalRegister
    if not isinstance(condition, tuple) or len(condition) != 2:
        raise NotImplementedError('Dynamic bridge requires a single-bit equality condition')
    bit, expected = condition
    if isinstance(bit, ClassicalRegister):
        if len(bit) != 1:
            raise NotImplementedError('Multi-bit register conditions require a separate bridge')
        bit = bit[0]
    if not isinstance(bit, Clbit) or expected not in (0, 1, False, True):
        raise NotImplementedError('Dynamic bridge requires a boolean classical bit')
    return circuit.find_bit(bit).index, int(expected)


def serialize_dynamic_qasm2(circuit):
    """Serialize a u3/cx circuit with flat single-bit if/else unitary bodies."""
    lines = ['OPENQASM 2.0;', 'include "qelib1.inc";', 'qreg q[%d];' % circuit.num_qubits]
    lines += ['creg c%d[1];' % i for i in range(circuit.num_clbits)]

    def emit(scope, qmap, cmap, predicate=None):
        for inst in scope.data:
            op = inst.operation
            qs = [qmap[scope.find_bit(q).index] for q in inst.qubits]
            cs = [cmap[scope.find_bit(c).index] for c in inst.clbits]
            prefix = '' if predicate is None else 'if(c%d==%d) ' % predicate
            if op.name == 'if_else':
                if predicate is not None:
                    raise NotImplementedError('Nested feedback is not supported by this bridge')
                index, value = single_bit_condition(op.condition, scope)
                condition = (cmap[index], value)
                for branch_index, body in enumerate(op.blocks):
                    if branch_index > 1:
                        raise NotImplementedError('More than two feedback branches')
                    # A QASM2 condition is evaluated for every emitted gate. Any
                    # classical write in a branch would change its semantics.
                    if any(i.clbits or i.operation.name in ('if_else','while_loop','for_loop','store')
                           for i in body.data):
                        raise NotImplementedError('Feedback body must not write classical bits or nest control flow')
                    emit(body, qs, cs, (condition[0], value if branch_index == 0 else 1-value))
            elif op.name == 'measure':
                if predicate is not None or len(qs) != 1 or len(cs) != 1:
                    raise NotImplementedError('Only unconditional single-qubit measurements are supported')
                lines.append('measure q[%d] -> c%d[0];' % (qs[0], cs[0]))
            elif op.name == 'reset':
                if len(qs) != 1:raise NotImplementedError('Reset arity')
                lines.append(prefix + 'reset q[%d];' % qs[0])
            elif op.name == 'barrier':
                if predicate is not None:raise NotImplementedError('Conditional barrier')
                lines.append('barrier ' + ','.join('q[%d]' % q for q in qs) + ';')
            elif op.name == 'u3' and len(qs) == 1 and len(op.params) == 3 and not cs:
                angles = [float(x) for x in op.params]
                if not all(math.isfinite(x) for x in angles):raise ValueError('Nonfinite gate parameter')
                lines.append(prefix + 'u3(%s) q[%d];' % (','.join(format(x,'.17g') for x in angles),qs[0]))
            elif op.name == 'cx' and len(qs) == 2 and not cs:
                lines.append(prefix + 'cx q[%d],q[%d];' % tuple(qs))
            else:
                raise NotImplementedError('Dynamic bridge operation: ' + op.name)
    emit(circuit, list(range(circuit.num_qubits)), list(range(circuit.num_clbits)))
    return '\n'.join(lines) + '\n'


def has_feedback(circuit):
    return any(i.operation.name in ('if_else','for_loop','while_loop','switch_case') for i in circuit.data)
