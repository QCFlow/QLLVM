OPENQASM 2.0;
include "qelib1.inc";
// 纠缠子线路 (2w+1=9 qubits, w=4)：b + 4 个 x_ent + 4 个 y_ent；与 felity.ipynb qc_sim 一致
// 仅测 b 与 4 个 x → 边际分布（笔记本中对 y 求和的结果）
// q[0]=b, q[1..4]=x_ent[0..3], q[5..8]=y_ent[0..3]

qreg q[9];
creg c[5];

h q[0];
h q[1];
h q[2];
h q[3];
h q[4];
barrier q[0], q[1], q[2], q[3], q[4], q[5], q[6], q[7], q[8];
cx q[1], q[5];
cx q[2], q[6];
cx q[3], q[7];
cx q[4], q[8];
cx q[0], q[5];
cx q[0], q[6];
cx q[0], q[7];
cx q[0], q[8];
barrier q[0], q[1], q[2], q[3], q[4], q[5], q[6], q[7], q[8];
h q[0];
h q[1];
h q[2];
h q[3];
h q[4];
barrier q[0], q[1], q[2], q[3], q[4], q[5], q[6], q[7], q[8];

measure q[0] -> c[0];
measure q[1] -> c[1];
measure q[2] -> c[2];
measure q[3] -> c[3];
measure q[4] -> c[4];
