"""Independent semantic regressions for the combined compiler repairs."""
import math
import os
from pathlib import Path
import subprocess
import tempfile
import unittest

from qiskit import QuantumCircuit, qasm2
from qiskit.quantum_info import Operator


@unittest.skipUnless(os.environ.get('QLLVM_TEST_COMPILER'), 'Set QLLVM_TEST_COMPILER')
class CompilerRepairTests(unittest.TestCase):
    def compile(self, text, opt):
        with tempfile.TemporaryDirectory() as folder:
            source, output = Path(folder)/'input.qasm', Path(folder)/'output.qasm'
            source.write_text(text)
            result = subprocess.run([os.environ['QLLVM_TEST_COMPILER'], str(source),
                '-'+opt, '-exact-rotations', '-mlir-disable-threading',
                '-emit-backend=qasm-backend', '-output-path='+str(output)],
                capture_output=True, text=True, timeout=30)
            return result, output.read_text() if output.exists() else ''

    def test_natural_logarithm_and_constant_arithmetic(self):
        text = 'OPENQASM 2.0; include "qelib1.inc"; qreg q[1]; h q[0]; rz(ln(2)+sin(pi/6)/2) q[0];'
        expected = QuantumCircuit(1)
        expected.h(0)
        expected.rz(math.log(2)+math.sin(math.pi/6)/2, 0)
        for opt in ['O0', 'O1']:
            with self.subTest(opt=opt):
                result, output = self.compile(text, opt)
                self.assertEqual(result.returncode, 0, result.stdout+result.stderr)
                actual = qasm2.loads(output, custom_instructions=qasm2.LEGACY_CUSTOM_INSTRUCTIONS)
                self.assertTrue(Operator(expected).equiv(Operator(actual), atol=1e-8, rtol=0))

    def test_cp_alias_with_both_control_directions(self):
        for opt in ['O0', 'O1']:
            for pair in [(0,1), (1,0)]:
                with self.subTest(opt=opt, pair=pair):
                    expected = QuantumCircuit(2)
                    expected.ry(.71, pair[0])
                    expected.cp(-.37, *pair)
                    result, output = self.compile(qasm2.dumps(expected), opt)
                    self.assertEqual(result.returncode, 0, result.stdout+result.stderr)
                    actual = qasm2.loads(output, custom_instructions=qasm2.LEGACY_CUSTOM_INSTRUCTIONS)
                    self.assertTrue(Operator(expected).equiv(Operator(actual), atol=1e-8, rtol=0))

    def test_unsupported_relational_predicates_fail_explicitly(self):
        for predicate in ['<', '>', '<=', '>=']:
            with self.subTest(predicate=predicate):
                text = 'OPENQASM 2.0; include "qelib1.inc"; qreg q[1]; creg c[1]; if(c'+predicate+'1) x q[0];'
                result, output = self.compile(text, 'O0')
                self.assertNotEqual(result.returncode, 0)
                self.assertIn('Unsupported classical predicate', result.stdout+result.stderr)
                self.assertFalse(output)


if __name__ == '__main__':
    unittest.main()
