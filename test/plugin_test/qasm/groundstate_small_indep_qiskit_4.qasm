// Benchmark was created by MQT Bench on 2024-03-19
// For more information about MQT Bench, please visit https://www.cda.cit.tum.de/mqtbench/
// MQT Bench version: 1.1.0
// Qiskit version: 1.0.2

OPENQASM 2.0;
include "qelib1.inc";
qreg q[4];
creg meas[4];
u2(0,1.3357698692274171) q[0];
u2(0,1.5967841156732785) q[1];
cz q[0],q[1];
u2(0,2.760259208932286) q[2];
cz q[0],q[2];
cz q[1],q[2];
u2(0,-0.7634435120289043) q[3];
cz q[0],q[3];
u2(0,0.8546761091641892) q[0];
cz q[1],q[3];
u2(0,0.7152156570197823) q[1];
cz q[0],q[1];
cz q[2],q[3];
u2(0,0.9493373039130883) q[2];
cz q[0],q[2];
cz q[1],q[2];
u2(0,1.1781896252048067) q[3];
cz q[0],q[3];
u2(0,-1.130944498160435) q[0];
cz q[1],q[3];
u2(0,1.7887653487954323) q[1];
cz q[2],q[3];
u2(0,-1.1315861573447377) q[2];
u2(0,2.7636383453160365) q[3];
barrier q[0],q[1],q[2],q[3];
measure q[0] -> meas[0];
measure q[1] -> meas[1];
measure q[2] -> meas[2];
measure q[3] -> meas[3];