OPENQASM 2.0;
include "qelib1.inc";
// 同一路径 MaxCut 问题（H_C = 0.5 Z0 Z1 + 0.5 Z1 Z2），但 QAOA p=3 使用「故意很小」的 γ,β。
// 态仍接近 |+++>，精确 <H_C> ≈ 0.036（远差于最优 -1）；采样下应在 8 种基矢上都有权重。
// 由 Qiskit QAOAAnsatz + 固定差参数转译得到。

qreg q[3];
creg c[3];

h q[0];
h q[1];
h q[2];
cx q[1], q[2];
rz(0.02) q[2];
cx q[1], q[2];
cx q[0], q[1];
rz(0.02) q[1];
cx q[0], q[1];
rx(0.08) q[0];
rx(0.08) q[1];
rx(0.08) q[2];
cx q[1], q[2];
rz(0.06) q[2];
cx q[1], q[2];
cx q[0], q[1];
rz(0.06) q[1];
cx q[0], q[1];
rx(0.06) q[0];
rx(0.06) q[1];
rx(0.06) q[2];
cx q[1], q[2];
rz(0.04) q[2];
cx q[1], q[2];
cx q[0], q[1];
rz(0.04) q[1];
cx q[0], q[1];
rx(0.1) q[0];
rx(0.1) q[1];
rx(0.1) q[2];

measure q[0] -> c[0];
measure q[1] -> c[1];
measure q[2] -> c[2];
