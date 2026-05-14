// Benchmark was created by MQT Bench on 2024-03-18
// For more information about MQT Bench, please visit https://www.cda.cit.tum.de/mqtbench/
// MQT Bench version: 1.1.0
// Qiskit version: 1.0.2
// Used Gate Set: ['id', 'rz', 'sx', 'x', 'cx', 'measure', 'barrier']

OPENQASM 2.0;
include "qelib1.inc";
qreg q[1];
qreg psi[1];
creg c[1];
rz(pi/2) q[0];
rz(-pi/2) q[0];
h q[0];
rz(-pi/2) q[0];
rz(pi/2) q[0];
rx(pi) psi[0];
rz(pi/4) psi[0];
h q[0];
cz psi[0],q[0];
h q[0];
rz(-pi/4) q[0];
h q[0];
cz psi[0],q[0];
h q[0];
rz(pi/4) q[0];
rz(pi/2) q[0];
rz(-pi/2) q[0];
h q[0];
rz(-pi/2) q[0];
rz(pi/2) q[0];
barrier q[0],psi[0];
measure q[0] -> c[0];
