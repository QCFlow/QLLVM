"""Conservative convergence preserves coherent circuits and dynamic channels."""
import os
import random
import unittest
from unittest.mock import patch
from qiskit import QuantumCircuit,qasm2
from qiskit.quantum_info import Operator
import test_qllvm_parser_regression as parser_tests
import sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[2]/'benchmark/official_suites'))
from instrument_verification import verify_instrument

@unittest.skipUnless(os.environ.get('QLLVM_TEST_COMPILER'),'Set QLLVM_TEST_COMPILER')
class OptimizationRoundTests(unittest.TestCase):
    def compile_both(self,text):
        outputs=[]
        for fixed in ['0','1']:
            with patch.dict(os.environ,QLLVM_FIXED_OPT_ROUNDS=fixed,QLLVM_AUDIT_MUTATIONS='1'):
                result,out=parser_tests.ParserRegressionTests.compile(self,text,'O1',45,
                    ['-mlir-disable-threading','-trace-timings'])
            self.assertEqual(result.returncode,0,result.stdout+result.stderr)
            self.assertNotIn('mutation-audit-mismatch',result.stderr)
            outputs.append((qasm2.loads(out,custom_instructions=qasm2.LEGACY_CUSTOM_INSTRUCTIONS),result.stderr))
        return outputs

    def test_stable_circuit_exits_early_and_fixed_schedule_is_available(self):
        c=QuantumCircuit(3);c.h(0);c.cx(0,1);c.cx(1,2)
        (a,trace),(b,fixed)=self.compile_both(qasm2.dumps(c))
        self.assertIn('unchanged=1',trace)
        self.assertLess(trace.count('[qllvm-round]'),4)
        self.assertEqual(fixed.count('[qllvm-round]'),4)
        self.assertTrue(Operator(c).equiv(Operator(a),atol=1e-7,rtol=0))
        self.assertTrue(Operator(a).equiv(Operator(b),atol=1e-7,rtol=0))

    def test_changed_angles_wire_order_and_overlapping_runs_keep_semantics(self):
        for seed in range(8):
            rng=random.Random(seed);c=QuantumCircuit(4)
            for k in range(50):
                a,b=rng.sample(range(4),2)
                c.rx(.37 if k%2 else .370000000001,a)
                c.rz(rng.uniform(-3,3),b);c.cx(a,b)
            (a,_),(b,_)=self.compile_both(qasm2.dumps(c))
            self.assertTrue(Operator(c).equiv(Operator(a),atol=1e-7,rtol=0))
            self.assertTrue(Operator(c).equiv(Operator(b),atol=1e-7,rtol=0))

    def test_measurement_reset_and_feedback_are_not_ignored_by_convergence(self):
        text=parser_tests.HEADER+'qreg q[2];\ncreg flag[1];\ncreg c[1];\nh q[0];\nmeasure q[0] -> flag[0];\nif(flag==1) x q[1];\nreset q[0];\nmeasure q[1] -> c[0];\n'
        (a,_),(b,_)=self.compile_both(text)
        original=qasm2.loads(text,custom_instructions=qasm2.LEGACY_CUSTOM_INSTRUCTIONS)
        for after in [a,b]:
            result=verify_instrument(original,after,tolerance=1e-7)
            self.assertEqual(result['status'],'passed',result)

    def test_reset_is_preserved_through_decomposition_and_target_basis(self):
        # Reset destroys the original state, including correlations with the
        # other wire. It must survive O0/O1 and must not be synthesized as a
        # unitary, even when the target basis contains no 'reset' gate.
        text=parser_tests.HEADER+('qreg q[2];\ncreg c[1];\nh q[0];\n'
            'cx q[0],q[1];\nreset q[0];\nry(0.37) q[0];\n'
            'cx q[0],q[1];\nmeasure q[1] -> c[0];\n')
        original=qasm2.loads(text,custom_instructions=qasm2.LEGACY_CUSTOM_INSTRUCTIONS)
        for opt in ['O0','O1']:
            with self.subTest(opt=opt):
                result,out=parser_tests.ParserRegressionTests.compile(self,text,opt,45,
                    ['-mlir-disable-threading'])
                self.assertEqual(result.returncode,0,result.stdout+result.stderr)
                after=qasm2.loads(out,custom_instructions=qasm2.LEGACY_CUSTOM_INSTRUCTIONS)
                self.assertEqual(after.count_ops().get('reset',0),1)
                check=verify_instrument(original,after,tolerance=1e-7)
                self.assertEqual(check['status'],'passed',check)
