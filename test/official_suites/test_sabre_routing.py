"""Check physical connectivity and full operators including layout permutations."""
import json
import os
from pathlib import Path
import subprocess
import tempfile
import unittest

import numpy as np
from qiskit import QuantumCircuit, qasm2
from qiskit.quantum_info import Operator


@unittest.skipUnless(os.environ.get('QLLVM_TEST_COMPILER'), 'Set QLLVM_TEST_COMPILER')
class SabreRoutingTests(unittest.TestCase):
    def compile(self, circuit, edges, extra=()):
        with tempfile.TemporaryDirectory() as folder:
            source, output, report = [Path(folder)/n for n in ['input.qasm','output.qasm','routing.json']]
            source.write_text(qasm2.dumps(circuit) if isinstance(circuit, QuantumCircuit) else circuit)
            result = subprocess.run([os.environ['QLLVM_TEST_COMPILER'], str(source), '-O0',
                '-mlir-disable-threading', '-emit-backend=qasm-backend',
                '-output-path='+str(output), '-sabre-report='+str(report),
                '-sabre-coupling-map='+';'.join(f'{a},{b}' for a,b in edges), *extra],
                capture_output=True, text=True, timeout=90)
            return result, output.read_text() if output.exists() else '', json.loads(report.read_text()) if report.exists() else None

    def test_routed_operator_connectivity_and_seed_reproducibility(self):
        source = QuantumCircuit(4)
        source.h(0)
        for a,b,t in [(0,3,.31),(3,1,-.17),(2,0,.43),(1,3,.29),(0,2,-.37)]:
            source.cx(a,b)
            source.ry(t,b)
            source.rz(t/2,a)
        edges = [(0,1),(1,2),(2,3)]
        original = Operator(source).data
        for heuristic in ['basic','lookahead','decay']:
            for seed in [7,43]:
                with self.subTest(heuristic=heuristic, seed=seed):
                    args = [f'-sabre-heuristic={heuristic}',f'-sabre-seed={seed}',
                            '-sabre-layout-trials=2','-sabre-swap-trials=3']
                    result, text, report = self.compile(source, edges, args)
                    self.assertEqual(result.returncode, 0, result.stdout+result.stderr)
                    after = qasm2.loads(text, custom_instructions=qasm2.LEGACY_CUSTOM_INSTRUCTIONS)
                    for instruction in after.data:
                        if len(instruction.qubits)==2:
                            a,b = [after.find_bit(q).index for q in instruction.qubits]
                            self.assertIn(tuple(sorted((a,b))), edges)
                    def permuted(layout):
                        return [sum(((index>>q)&1)<<layout[q] for q in range(4)) for index in range(16)]
                    expected = np.zeros_like(original)
                    expected[np.ix_(permuted(report['final_layout']),permuted(report['initial_layout']))] = original
                    self.assertTrue(Operator(expected).equiv(Operator(after),atol=1e-8,rtol=0))
                    repeated, repeated_text, repeated_report = self.compile(source, edges, args)
                    self.assertEqual(repeated.returncode, 0, repeated.stdout+repeated.stderr)
                    self.assertEqual(text, repeated_text)
                    self.assertEqual(report, repeated_report)

    def test_invalid_topologies_and_layouts_are_rejected(self):
        source = QuantumCircuit(3)
        source.cx(0,2)
        for edges, extra, message in [
            ([(0,1)], [], 'fewer qubits'),
            ([(0,1),(2,3)], [], 'connected coupling graph'),
            ([(0,1),(1,2)], ['-sabre-initial-layout=0,0,2'], 'permute all physical indices'),
            ([(0,0)], [], 'distinct coupling endpoints')]:
            with self.subTest(message=message):
                result, text, report = self.compile(source, edges, extra)
                self.assertNotEqual(result.returncode, 0)
                self.assertIn(message, result.stdout+result.stderr)
                self.assertFalse(text)

    def test_classical_control_flow_is_not_flattened(self):
        source = 'OPENQASM 2.0; include "qelib1.inc"; qreg q[2]; creg c[1]; measure q[0] -> c[0]; if(c==1) x q[1];'
        result, text, report = self.compile(source, [(0,1)])
        self.assertNotEqual(result.returncode, 0)
        self.assertIn('control-flow restriction',result.stdout+result.stderr)
        self.assertFalse(text)


if __name__ == '__main__':
    unittest.main()
