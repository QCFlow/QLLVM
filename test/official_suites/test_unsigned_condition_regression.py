"""Exact unsigned whole-register conditions, including values above INT64_MAX."""
import os,re,unittest
import test_qllvm_parser_regression as parser_tests
from qiskit import qasm2

@unittest.skipUnless(os.environ.get('QLLVM_TEST_COMPILER'), 'Set QLLVM_TEST_COMPILER')
class UnsignedConditionTests(unittest.TestCase):
    def test_64_bit_comparison_roundtrips_without_float_rounding(self):
        values=[0,1,2**53+1,2**63-1,2**63,2**63+1,2**64-1]
        for opt in ('O0','O1'):
            with self.subTest(opt=opt):
                text='OPENQASM 2.0;\ninclude "qelib1.inc";\nqreg q[2];\ncreg c[64];\nmeasure q[0] -> c[63];\n'
                text+='\n'.join('if(c==%d) x q[1];'%v for v in values)
                result,output=parser_tests.ParserRegressionTests.compile(self,text,opt)
                self.assertEqual(result.returncode,0,result.stdout+result.stderr)
                self.assertEqual([int(x) for x in re.findall(r'if\s*\(\s*\w+\s*==\s*(\d+)\s*\)',output)],values)
                circuit=qasm2.loads(output,custom_instructions=qasm2.LEGACY_CUSTOM_INSTRUCTIONS)
                self.assertEqual(circuit.count_ops().get('if_else'),len(values))
                measurement=next(i for i in circuit.data if i.operation.name=='measure')
                self.assertEqual(circuit.find_bit(measurement.clbits[0]).index,63)

    def test_constant_over_uint64_is_rejected(self):
        text='OPENQASM 2.0; include "qelib1.inc"; qreg q[1]; creg c[64]; if(c==18446744073709551616) x q[0];'
        result,output=parser_tests.ParserRegressionTests.compile(self,text)
        self.assertNotEqual(result.returncode,0)
        self.assertIn('64-bit unsigned range',result.stdout+result.stderr)
        self.assertFalse(output)

if __name__=='__main__':unittest.main()
