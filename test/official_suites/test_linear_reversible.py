"""Bounded exact three-wire rewrites preserve operators and dynamic boundaries."""
import os
import random
import unittest
import sys
from pathlib import Path
from qiskit import QuantumCircuit,qasm2
from qiskit.quantum_info import Operator
import test_qllvm_parser_regression as parser_tests
sys.path.insert(0,str(Path(__file__).resolve().parents[2]/'benchmark/official_suites'))
from instrument_verification import verify_instrument

@unittest.skipUnless(os.environ.get('QLLVM_TEST_COMPILER'),'Set QLLVM_TEST_COMPILER')
class LinearReversibleTests(unittest.TestCase):
    def compile(self,c):
        done,text=parser_tests.ParserRegressionTests.compile(self,qasm2.dumps(c),'O1',45,
            ['-mlir-disable-threading','-exact-rotations'])
        self.assertEqual(done.returncode,0,done.stdout+done.stderr)
        return qasm2.loads(text,custom_instructions=qasm2.LEGACY_CUSTOM_INSTRUCTIONS)

    def test_cnot_commutator_reaches_one_entangler(self):
        for a,b,c in ((0,1,2),(2,0,1),(1,2,0)):
            circuit=QuantumCircuit(3)
            for edge in ((a,b),(b,c),(a,b),(b,c)):circuit.cx(*edge)
            after=self.compile(circuit)
            self.assertTrue(Operator(circuit).equiv(Operator(after),atol=1e-9,rtol=0))
            self.assertEqual(after.count_ops().get('cz',0),1)

    def test_random_three_wire_blocks_interrupted_by_rotations(self):
        for seed in range(12):
            rng=random.Random(seed);c=QuantumCircuit(4)
            for _ in range(5):
                for k in range(24):c.cx(*rng.sample(range(3),2))
                c.rz(.37,1);c.cx(2,3);c.rx(.21,0)
            after=self.compile(c)
            self.assertTrue(Operator(c).equiv(Operator(after),atol=1e-8,rtol=0))

    def test_measurement_reset_and_feedback_stop_linear_windows(self):
        c=QuantumCircuit(3,1);c.h(0)
        c.cx(0,1);c.cx(1,2);c.measure(2,0);c.reset(1)
        with c.if_test((c.cregs[0],1)):c.x(0)
        c.cx(0,1);c.cx(1,2);c.measure(0,0)
        after=self.compile(c)
        self.assertEqual(verify_instrument(c,after,tolerance=1e-8)['status'],'passed')

    def test_many_windows_on_deep_wires_preserve_boundary_mappings(self):
        c=QuantumCircuit(4)
        for _ in range(1600):
            for edge in ((0,1),(1,2),(0,1),(1,2)):c.cx(*edge)
            c.rz(.37,2);c.cx(2,3);c.rx(.21,0)
        after=self.compile(c)
        self.assertTrue(Operator(c).equiv(Operator(after),atol=1e-7,rtol=0))
