"""Independent operator checks also require useful two-qubit resynthesis."""
import os
import random
from pathlib import Path
import sys
import unittest

from qiskit import QuantumCircuit, qasm2
from qiskit.quantum_info import Operator
import test_qllvm_parser_regression as parser_tests
sys.path.insert(0, str(Path(__file__).resolve().parents[2] / 'benchmark/official_suites'))
from instrument_verification import verify_instrument


@unittest.skipUnless(os.environ.get('QLLVM_TEST_COMPILER'), 'Set QLLVM_TEST_COMPILER')
class KAKSynthesisTests(unittest.TestCase):
    def compile(self, circuit):
        result, text = parser_tests.ParserRegressionTests.compile(
            self, qasm2.dumps(circuit), 'O1', 60,
            ['-mlir-disable-threading', '-exact-rotations'])
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        compiled = qasm2.loads(text, custom_instructions=qasm2.LEGACY_CUSTOM_INSTRUCTIONS)
        self.assertTrue(Operator(circuit).equiv(Operator(compiled), atol=1e-7, rtol=0), text)
        return compiled

    def two(self, circuit):
        return sum(i.operation.num_qubits == 2 for i in circuit.data)

    def test_random_dense_two_qubit_blocks_reach_three_entanglers(self):
        for seed in range(32):
            with self.subTest(seed=seed):
                rng = random.Random(8241 + seed)
                c = QuantumCircuit(2)
                for k in range(12):
                    for q in range(2):
                        c.u(*(rng.uniform(-3, 3) for _ in range(3)), q)
                    c.cx(k % 2, 1 - k % 2)
                c.global_phase = .23 * seed
                self.assertLessEqual(self.two(self.compile(c)), 3)

    def test_degenerate_ising_blocks_remain_exact_and_are_compressed(self):
        for axis in ('rxx', 'ryy', 'rzz'):
            for theta in (0., .37, -1.1, 1.5707963267948966, 1e-7):
                with self.subTest(axis=axis, theta=theta):
                    c = QuantumCircuit(2)
                    for _ in range(5):
                        getattr(c, axis)(theta, 0, 1)
                    # Force both wires' local input/output bases to be visible.
                    c.rx(.17, 0); c.rz(-.29, 1)
                    self.assertLessEqual(self.two(self.compile(c)), 3)

    def test_reversed_operands_and_independent_pairs_preserve_tensor_order(self):
        c = QuantumCircuit(4)
        for k in range(12):
            c.u(.2+k*.01, -.4, .7, 3); c.u(-.3, .6, -.2, 2)
            c.cx(3, 2); c.u(.1, -.8, .9, 0); c.cx(0, 1)
        self.assertLessEqual(self.two(self.compile(c)), 6)

    def test_synthesis_does_not_cross_reset_measurement_or_feedback(self):
        for dynamic in (False, True):
            with self.subTest(dynamic=dynamic):
                c = QuantumCircuit(2, 1)
                c.h(0); c.cx(0, 1); c.rz(.23, 1); c.cx(1, 0)
                if dynamic:
                    c.measure(0, 0)
                    with c.if_test((c.cregs[0], 1)):
                        c.x(1)
                c.reset(0)
                c.cx(0, 1); c.ry(.31, 0); c.cx(1, 0); c.measure(1, 0)
                result, text = parser_tests.ParserRegressionTests.compile(
                    self, qasm2.dumps(c), 'O1', 60,
                    ['-mlir-disable-threading', '-exact-rotations'])
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                after = qasm2.loads(text, custom_instructions=qasm2.LEGACY_CUSTOM_INSTRUCTIONS)
                check = verify_instrument(c, after)
                self.assertEqual(check['status'], 'passed', check)

    def test_zero_one_two_three_entangler_classes(self):
        for expected in (0, 1, 2, 3):
            with self.subTest(expected=expected):
                c = QuantumCircuit(2)
                c.u(.23, -.51, .91, 0); c.u(-.37, .19, -.83, 1)
                if expected == 0:
                    for _ in range(4):
                        c.cx(0, 1); c.cx(0, 1)
                elif expected == 1:
                    c.cx(0, 1); c.cz(0, 1)
                elif expected == 2:
                    c.rzz(.73, 0, 1)
                else:
                    c.rxx(.21, 0, 1); c.ryy(.38, 0, 1); c.rzz(.57, 0, 1)
                c.u(-.29, .61, .43, 0); c.u(.17, -.71, .13, 1)
                self.assertEqual(self.two(self.compile(c)), expected)

    def test_independent_processes_emit_identical_circuit(self):
        rng = random.Random(1904)
        c = QuantumCircuit(4)
        for k in range(60):
            a, b = rng.sample(range(4), 2)
            c.rz(.37 if k % 3 else .370000000001, a)
            c.rx(rng.uniform(-2, 2), b); c.cx(a, b)
        outputs = [qasm2.dumps(self.compile(c)) for _ in range(5)]
        self.assertEqual(len(set(outputs)), 1)
