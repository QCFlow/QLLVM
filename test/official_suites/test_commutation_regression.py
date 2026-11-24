"""Semantic regressions for exact commutation caching and repeated groups."""
import os
from pathlib import Path
import random
import re
import subprocess
import tempfile
import unittest

from qiskit import QuantumCircuit, QuantumRegister, qasm2
from qiskit.quantum_info import Operator

COMPILER = os.environ.get('QLLVM_TEST_COMPILER')


@unittest.skipUnless(COMPILER, 'Set QLLVM_TEST_COMPILER')
class CommutationRegressionTests(unittest.TestCase):
    def compile(self, circuit, sequence='2\n'):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            source, output, passes = (root / x for x in ('in.qasm', 'out.qasm', 'passes.txt'))
            source.write_text(qasm2.dumps(circuit))
            passes.write_text(sequence)
            result = subprocess.run([COMPILER, str(source), '-O1',
                '-mlir-disable-threading', '-customPassSequence=' + str(passes),
                '-emit-backend=qasm-backend', '-output-path=' + str(output),
                '-basicgate=[rx,ry,rz,cz,h,x,y,z]'], capture_output=True, text=True, timeout=45,
                env=dict(os.environ, QLLVM_COMMUTATION_STATS='1'))
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            self.analysis_stats = [dict((key, int(value)) for key, value in
                re.findall(r'(family_probes|matrix_checks|cache_hits)=(\d+)', line))
                for line in result.stderr.splitlines() if '[qllvm-commutation]' in line]
            return qasm2.loads(output.read_text(), custom_instructions=qasm2.LEGACY_CUSTOM_INSTRUCTIONS)

    def assert_unitary(self, circuit):
        compiled = self.compile(circuit)
        self.assertTrue(Operator(circuit).equiv(Operator(compiled), atol=1e-8, rtol=0),
                        qasm2.dumps(circuit) + '\nOUTPUT\n' + qasm2.dumps(compiled))
        return compiled

    def test_operand_roles_and_angle_bits_do_not_alias(self):
        for seed in range(24):
            with self.subTest(seed=seed):
                rng = random.Random(seed)
                circuit = QuantumCircuit(4)
                for _ in range(80):
                    a, b = rng.sample(range(4), 2)
                    name = rng.choice(['h', 'x', 'y', 'z', 'rx', 'ry', 'rz', 'p', 'u', 'cx', 'cy', 'cz', 'swap'])
                    if name in ('cx', 'cy', 'cz', 'swap'):
                        getattr(circuit, name)(a, b)
                    elif name == 'u':
                        circuit.u(rng.random(), rng.random(), rng.random(), a)
                    elif name in ('rx', 'ry', 'rz', 'p'):
                        getattr(circuit, name)(rng.choice([0., 0.37, -0.37, 0.370000000001, 1.1]), a)
                    else:
                        getattr(circuit, name)(a)
                self.assert_unitary(circuit)

    def test_repeated_control_target_patterns(self):
        circuit = QuantumCircuit(4)
        for _ in range(20):
            circuit.cx(0, 1)
            circuit.cx(0, 2)
            circuit.cx(3, 1)
            circuit.rz(0.2, 0)
            circuit.rx(0.4, 1)
        circuit.cx(1, 0)  # Reversing control and target changes commutation.
        self.assert_unitary(circuit)

    def test_identical_entanglers_cancel_across_other_commuting_supports(self):
        for gate in ('cx','cy','cz'):
            for edges in [((0,1),(0,2)),((0,2),(1,2))]:
                with self.subTest(gate=gate,edges=edges):
                    c=QuantumCircuit(3)
                    for pair in (edges[0],edges[1],edges[0]):getattr(c,gate)(*pair)
                    # CY is first lowered to Sdg/CX/S; the first scan folds
                    # its inverse phases, the second exposes the CX pair.
                    after=self.compile(c,sequence='2\n2\n' if gate=='cy' else '2\n')
                    self.assertTrue(Operator(c).equiv(Operator(after),atol=1e-8,rtol=0))
                    self.assertEqual(after.count_ops().get('cz',0),1)

    def test_other_wire_commutation_boundary_blocks_cancellation(self):
        for middle in [('cx',(1,2)),('cx',(1,0)),('cz',(1,2))]:
            with self.subTest(middle=middle):
                c=QuantumCircuit(3);c.cx(0,1)
                getattr(c,middle[0])(*middle[1]);c.cx(0,1)
                after=self.assert_unitary(c)
                self.assertEqual(after.count_ops().get('cz',0),3)

    def test_interleaved_many_supports_preserve_parity_and_order(self):
        c=QuantumCircuit(5)
        for target in (1,2,3,4,1,3,2,4,2):c.cx(0,target)
        after=self.assert_unitary(c)
        self.assertEqual(after.count_ops().get('cz',0),1)

    def test_unique_angle_diagonal_group_and_noncommuting_boundary(self):
        circuit = QuantumCircuit(2)
        for i in range(1200):
            circuit.rz((i + 1) * 0.0001, 0)
            circuit.cz(0, 1)
        circuit.h(0)
        circuit.rz(0.21, 0)
        self.assert_unitary(circuit)

    def test_long_repeated_group_is_fully_cancelled(self):
        circuit = QuantumCircuit(1)
        for _ in range(40000):
            circuit.h(0)
        compiled = self.compile(circuit)
        self.assertEqual(len(compiled.data), 0)

    def test_measurement_is_not_crossed(self):
        circuit = QuantumCircuit(1, 1)
        circuit.h(0)
        circuit.measure(0, 0)
        circuit.h(0)
        compiled = self.compile(circuit)
        self.assertEqual([i.operation.name for i in compiled.data], ['h', 'measure', 'h'])

    def test_distinct_angle_families_have_linear_analysis(self):
        for axis in ('rx', 'ry', 'rz'):
            with self.subTest(axis=axis):
                circuit = QuantumCircuit(1)
                for i in range(4000):
                    getattr(circuit, axis)((i + 1) * 0.000123, 0)
                self.assert_unitary(circuit)
                self.assertTrue(self.analysis_stats)
                self.assertLessEqual(sum(s['family_probes'] for s in self.analysis_stats), 4000)
                self.assertEqual(sum(s['matrix_checks'] for s in self.analysis_stats), 0)

    def test_controlled_gate_axis_families_preserve_operand_roles(self):
        for gate, axis, target in [('cx', 'rx', 1), ('cy', 'ry', 1),
                                   ('cx', 'rz', 0), ('cy', 'rz', 0),
                                   ('cz', 'rz', 0), ('cz', 'rz', 1)]:
            with self.subTest(gate=gate, axis=axis, target=target):
                circuit = QuantumCircuit(2)
                for i in range(200):
                    getattr(circuit, gate)(0, 1)
                    getattr(circuit, axis)((i + 1) * 0.00123, target)
                # Same rotation on the other role is not generally commuting.
                circuit.h(0)
                getattr(circuit, axis)(0.21, 1 - target)
                getattr(circuit, gate)(1, 0)
                self.assert_unitary(circuit)

    def test_special_angles_do_not_prove_the_entire_family(self):
        for axis in ('rx', 'ry'):
            circuit = QuantumCircuit(2)
            getattr(circuit, axis)(0.0, 0)
            getattr(circuit, axis)(0.37, 0)
            circuit.rz(0.28, 0)
            getattr(circuit, axis)(-0.37, 0)
            circuit.cx(0, 1)
            self.assert_unitary(circuit)

    def test_multiple_register_roots_and_repeated_analysis(self):
        left, right = QuantumRegister(2, 'left'), QuantumRegister(2, 'right')
        circuit = QuantumCircuit(left, right)
        # The first operation starts two SSA paths, in reverse register order.
        circuit.cx(right[1], left[0])
        circuit.rz(0.27, right[1])
        circuit.cx(right[1], left[0])
        circuit.h(left[1])
        circuit.h(left[1])
        circuit.ry(0.31, right[0])
        circuit.cx(left[0], right[0])
        compiled = self.compile(circuit, sequence='2\n2\n')
        self.assertTrue(Operator(circuit).equiv(Operator(compiled), atol=1e-8, rtol=0))
        self.assertEqual(len(self.analysis_stats), 2)


if __name__ == '__main__':
    unittest.main()
