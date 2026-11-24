"""Flat-QASM chunks preserve parsing, scope, errors and quantum wire identity."""
import os
import unittest
import re
from unittest.mock import patch
from qiskit import qasm2
from qiskit.quantum_info import Operator
import test_qllvm_parser_regression as parser_tests

@unittest.skipUnless(os.environ.get('QLLVM_TEST_COMPILER'),'Set QLLVM_TEST_COMPILER')
class ChunkedFrontendTests(unittest.TestCase):
    def compare(self,text):
        outputs=[]
        for disabled in ('0','1'):
            with patch.dict(os.environ,QLLVM_DISABLE_CHUNKED_QASM=disabled):
                done,out=parser_tests.ParserRegressionTests.compile(self,text,'O0',90,
                    ['-mlir-disable-threading','-trace-timings'])
            self.assertEqual(done.returncode,0,done.stdout+done.stderr)
            outputs.append((out,done.stderr))
        self.assertEqual(outputs[0][0],outputs[1][0])
        return outputs

    def test_flat_parameters_comments_multiline_statements_and_registers(self):
        text=parser_tests.HEADER+'qreg left[1];\nqreg right[1];\n'
        text+='''// semicolons ; and braces { } in comments are harmless.
/* pi; multiline */ u3(
2e-2, -0.3, 0.7) left[0]; cx left[0],right[0];
'''*2500
        outputs=self.compare(text)
        self.assertIn('chunked=1',outputs[0][1])
        reference=re.sub(r'/\*.*?\*/','',text,flags=re.S)
        original=qasm2.loads(reference,custom_instructions=qasm2.LEGACY_CUSTOM_INSTRUCTIONS)
        after=qasm2.loads(outputs[0][0],custom_instructions=qasm2.LEGACY_CUSTOM_INSTRUCTIONS)
        self.assertTrue(Operator(original).equiv(Operator(after),atol=1e-7,rtol=0))

    def test_custom_gate_and_feedback_use_the_general_parser(self):
        text=parser_tests.HEADER+'gate custom(a) q { rx(a) q; }\nqreg q[1];\ncreg c[1];\n'
        text+='custom(.37) q[0];\n'*4000+'measure q[0] -> c[0];\nif(c==1) x q[0];\n'
        outputs=self.compare(text)
        self.assertNotIn('chunked=1',outputs[0][1])

    def test_late_malformed_statement_is_rejected_with_original_line(self):
        text=parser_tests.HEADER+'qreg q[1];\n'+'rx(0.123456789) q[0];\n'*5000
        bad_line=text.count('\n')+1
        text+='rx(0.2,,0.3) q[0];\n'
        with patch.dict(os.environ,QLLVM_DISABLE_CHUNKED_QASM='0'):
            done,out=parser_tests.ParserRegressionTests.compile(self,text,'O0',90,
                ['-mlir-disable-threading'])
        self.assertNotEqual(done.returncode,0)
        self.assertIn(':'+str(bad_line)+':',done.stdout+done.stderr)
