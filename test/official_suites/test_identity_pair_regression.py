"""Isolated cleanup passes retain wire and nonunitary boundaries on long runs."""
import os
from pathlib import Path
import tempfile
import unittest
import sys
from unittest.mock import patch
from qiskit import QuantumCircuit, qasm2
from qiskit.quantum_info import Operator
import test_qllvm_parser_regression as parser_tests
sys.path.insert(0,str(Path(__file__).resolve().parents[2]/'benchmark/official_suites'))
from instrument_verification import verify_instrument


@unittest.skipUnless(os.environ.get('QLLVM_TEST_COMPILER'), 'Set QLLVM_TEST_COMPILER')
class IdentityPairTests(unittest.TestCase):
    def test_default_worklist_cleans_nested_pairs_and_reset_chain_in_one_round(self):
        c=QuantumCircuit(3,1)
        c.h(0);c.s(0);c.t(0);c.tdg(0);c.sdg(0);c.h(0)
        for _ in range(65):c.reset(1)
        c.x(1);c.reset(1);c.cx(2,1);c.cx(1,2);c.measure(1,0)
        outputs=[]
        for legacy in ('0','1'):
            with patch.dict(os.environ,QLLVM_LEGACY_LOCAL_CLEANUP=legacy):
                done,text=parser_tests.ParserRegressionTests.compile(self,qasm2.dumps(c),'O1',60,
                    ['-mlir-disable-threading','-exact-rotations'])
            self.assertEqual(done.returncode,0,done.stdout+done.stderr)
            after=qasm2.loads(text,custom_instructions=qasm2.LEGACY_CUSTOM_INSTRUCTIONS)
            self.assertEqual(verify_instrument(c,after,tolerance=1e-8)['status'],'passed')
            outputs.append(after)
        self.assertEqual(outputs[0].count_ops().get('reset',0),2)
        self.assertLessEqual(len(outputs[0].data),len(outputs[1].data))

    def compile_pass(self, circuit, sequence):
        with tempfile.TemporaryDirectory() as tmp:
            passes=Path(tmp)/'passes.txt';passes.write_text(sequence)
            done,text=parser_tests.ParserRegressionTests.compile(self,qasm2.dumps(circuit),'O1',60,
                ['-mlir-disable-threading','-customPassSequence='+str(passes),'-exact-rotations'])
        self.assertEqual(done.returncode,0,done.stdout+done.stderr)
        return qasm2.loads(text,custom_instructions=qasm2.LEGACY_CUSTOM_INSTRUCTIONS)

    def test_long_disjoint_inverse_pairs_leave_only_uncancelled_tails(self):
        for gate,sequence in [('h','3\n'),('cx','4\n')]:
            with self.subTest(gate=gate):
                c=QuantumCircuit(4)
                for _ in range(2000):
                    getattr(c,gate)(*([0,1] if gate=='cx' else [0]))
                    getattr(c,gate)(*([2,3] if gate=='cx' else [2]))
                getattr(c,gate)(*([0,1] if gate=='cx' else [0]))
                after=self.compile_pass(c,sequence)
                self.assertTrue(Operator(c).equiv(Operator(after),atol=1e-9,rtol=0))
                self.assertEqual(after.count_ops().get('cz' if gate=='cx' else gate,0),1)

    def test_reset_cleanup_preserves_instrument_and_intervening_gate(self):
        c=QuantumCircuit(2,1);c.h(0);c.cx(0,1)
        for _ in range(101):c.reset(0)
        c.x(0);c.reset(0);c.measure(1,0)
        # This pass preserves its existing one-scan matching schedule; repeated
        # scans expose newly adjacent reset pairs without crossing X.
        after=self.compile_pass(c,'5\n'*7)
        self.assertEqual(after.count_ops().get('reset',0),2)
        self.assertEqual(verify_instrument(c,after,tolerance=1e-8)['status'],'passed')
