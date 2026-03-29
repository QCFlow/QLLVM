// Benchmark was created by MQT Bench on 2024-03-18
// For more information about MQT Bench, please visit https://www.cda.cit.tum.de/mqtbench/
// MQT Bench version: 1.1.0
// Qiskit version: 1.0.2
// Used Gate Set: ['id', 'rz', 'sx', 'x', 'cx', 'measure', 'barrier']

OPENQASM 2.0;
include "qelib1.inc";
qreg q[3];
creg meas[3];
rz(pi/2) q[2];
rz(-pi/2) q[2];
h q[2];
rz(-pi/2) q[2];
rz(pi/2) q[2];
rz(pi/4) q[2];
h q[1];
cz q[2],q[1];
h q[1];
rz(-pi/4) q[1];
h q[1];
cz q[2],q[1];
h q[1];
rz(pi/4) q[1];
rz(pi/2) q[1];
rz(-pi/2) q[1];
h q[1];
rz(-pi/2) q[1];
rz(pi/2) q[1];
rz(pi/4) q[1];
rz(pi/8) q[2];
h q[0];
cz q[2],q[0];
h q[0];
rz(-pi/8) q[0];
h q[0];
cz q[2],q[0];
h q[0];
rz(pi/8) q[0];
h q[0];
cz q[1],q[0];
h q[0];
rz(-pi/4) q[0];
h q[0];
cz q[1],q[0];
h q[0];
rz(pi/4) q[0];
rz(pi/2) q[0];
rz(-pi/2) q[0];
h q[0];
rz(-pi/2) q[0];
rz(pi/2) q[0];
h q[2];
cz q[0],q[2];
h q[2];
h q[0];
cz q[2],q[0];
h q[0];
h q[2];
cz q[0],q[2];
h q[2];
barrier q[0],q[1],q[2];
measure q[0] -> meas[0];
measure q[1] -> meas[1];
measure q[2] -> meas[2];
