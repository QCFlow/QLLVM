OPENQASM 2.0;
include "qelib1.inc";
qreg q[2];
creg c[2];

// Hardware-efficient-style ansatz: alternating single-qubit rotations and entanglement.
// Angles are fixed constants (hybrid build bakes them into .bc).

// Block A — local mixing + linear entanglement
ry(0.35) q[0];
ry(0.55) q[1];
cx q[0], q[1];
rz(0.22) q[0];
rz(0.48) q[1];

// Block B — ring-style CNOT and another rotation layer
cx q[1], q[0];
ry(0.62) q[0];
ry(0.18) q[1];
cx q[0], q[1];

// Block C — general single-qubit unitary on q[0], phase on q[1]
u3(0.95, 0.41, 0.27) q[0];
rz(0.71) q[1];
cx q[1], q[0];

// Block D — final rotation + entanglement (Z-basis measure both qubits for Pauli readout)
ry(0.29) q[0];
ry(0.83) q[1];
cx q[0], q[1];

measure q[0] -> c[0];
measure q[1] -> c[1];
