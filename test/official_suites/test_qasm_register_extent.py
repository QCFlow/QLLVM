"""Exported QASM preserves idle inputs and original multi-register indexing."""
import os
import unittest
from qiskit import QuantumCircuit, QuantumRegister, qasm2
from qiskit.quantum_info import Operator
import test_qllvm_parser_regression as parser_tests


@unittest.skipUnless(os.environ.get('QLLVM_TEST_COMPILER'),'Set QLLVM_TEST_COMPILER')
class RegisterExtentTests(unittest.TestCase):
    def test_identity_and_empty_circuits_keep_all_declared_qubits(self):
        for pair in (False,True):
            c=QuantumCircuit(3)
            if pair:c.h(2);c.h(2)
            done,text=parser_tests.ParserRegressionTests.compile(self,qasm2.dumps(c),'O1',45,
                ['-mlir-disable-threading','-exact-rotations'])
            self.assertEqual(done.returncode,0,done.stdout+done.stderr)
            after=qasm2.loads(text,custom_instructions=qasm2.LEGACY_CUSTOM_INSTRUCTIONS)
            self.assertEqual(after.num_qubits,3)
            self.assertTrue(Operator(c).equiv(Operator(after),atol=1e-9,rtol=0))

    def test_idle_first_register_does_not_shift_active_second_register(self):
        c=QuantumCircuit(QuantumRegister(2,'idle'),QuantumRegister(2,'active'))
        c.ry(.37,3)
        done,text=parser_tests.ParserRegressionTests.compile(self,qasm2.dumps(c),'O1',45,
            ['-mlir-disable-threading','-exact-rotations'])
        self.assertEqual(done.returncode,0,done.stdout+done.stderr)
        after=qasm2.loads(text,custom_instructions=qasm2.LEGACY_CUSTOM_INSTRUCTIONS)
        self.assertEqual(after.num_qubits,4)
        self.assertTrue(Operator(c).equiv(Operator(after),atol=1e-9,rtol=0))
