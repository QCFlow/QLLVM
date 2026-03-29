// Benchmark was created by MQT Bench on 2024-03-17
// For more information about MQT Bench, please visit https://www.cda.cit.tum.de/mqtbench/
// MQT Bench version: 1.1.0
// Qiskit version: 1.0.2
// Used Gate Set: ['id', 'rz', 'sx', 'x', 'cx', 'measure', 'barrier']

OPENQASM 2.0;
include "qelib1.inc";
qreg eval[1];
qreg q[1];
creg meas[2];
rz(pi/2) eval[0];
rz(-pi/2) eval[0];
h eval[0];
rz(-pi/2) eval[0];
rz(pi/2) eval[0];
rz(0) eval[0];
rz(0) q[0];
rz(-pi/2) q[0];
h q[0];
rz(-pi/2) q[0];
rz(4.068887871591405) q[0];
rz(-pi/2) q[0];
h q[0];
rz(-pi/2) q[0];
rz(3*pi) q[0];
rz(0) q[0];
h q[0];
cz eval[0],q[0];
h q[0];
rz(0) q[0];
rz(-pi/2) q[0];
h q[0];
rz(-pi/2) q[0];
rz(2.214297435588181) q[0];
rz(-pi/2) q[0];
h q[0];
rz(-pi/2) q[0];
rz(3*pi) q[0];
h q[0];
cz eval[0],q[0];
h q[0];
rz(pi/2) eval[0];
rz(-pi/2) eval[0];
h eval[0];
rz(-pi/2) eval[0];
rz(pi/2) eval[0];
rz(0) q[0];
rz(-pi/2) q[0];
h q[0];
rz(-pi/2) q[0];
rz(4.068887871591405) q[0];
rz(-pi/2) q[0];
h q[0];
rz(-pi/2) q[0];
rz(3*pi) q[0];
barrier eval[0],q[0];
measure eval[0] -> meas[0];
measure q[0] -> meas[1];
