"""Direct QIR fixtures prevent upstream constant folding hiding exporter bugs.

Build qir_export_probe.cpp with qllvm-backend and llvm-config core/irreader/analysis
libraries; set QLLVM_TEST_QIR_EXPORTER to that executable.
"""
import math
import os
from pathlib import Path
import re
import subprocess
import tempfile
import unittest

HEADER = '''%Qubit = type opaque
%Array = type opaque
declare %Array* @__quantum__rt__qubit_allocate_array(i64)
declare i8* @__quantum__rt__array_get_element_ptr_1d(%Array*, i64)
declare void @__quantum__qis__rx(double, %Qubit*)
declare void @__quantum__qis__barrier(%Qubit*)
declare void @__quantum__qis__id(%Qubit*)
declare void @__quantum__qis__unknown(%Qubit*)
declare double @sin(double)
define void @__internal_mlir_probe(double %parameter, i1 %flag) {
entry:
  %array = call %Array* @__quantum__rt__qubit_allocate_array(i64 1)
  %element = call i8* @__quantum__rt__array_get_element_ptr_1d(%Array* %array, i64 0)
  %pointer = bitcast i8* %element to %Qubit**
  %q = load %Qubit*, %Qubit** %pointer
'''


@unittest.skipUnless(os.environ.get('QLLVM_TEST_QIR_EXPORTER'),'Set QLLVM_TEST_QIR_EXPORTER')
class QirExportRepairTests(unittest.TestCase):
    def export(self, body):
        with tempfile.TemporaryDirectory() as folder:
            path=Path(folder)/'input.ll'
            path.write_text(HEADER+body+'\n}\n')
            return subprocess.run([os.environ['QLLVM_TEST_QIR_EXPORTER'],str(path)],
                capture_output=True,text=True,timeout=15)

    def test_constant_expression_and_cast_semantics(self):
        for expression, expected in [
            ('%a = call double @sin(double 5.000000e-01)\n %angle = fadd double %a, 2.500000e-01', math.sin(.5)+.25),
            ('%angle = uitofp i64 -1 to double', float(2**64-1)),
            ('%angle = sitofp i64 -1 to double', -1.)]:
            with self.subTest(expression=expression):
                result=self.export(expression+'\ncall void @__quantum__qis__rx(double %angle, %Qubit* %q)\nret void')
                self.assertEqual(result.returncode,0,result.stderr)
                angle=float(re.search(r'rx\(([^)]+)\)',result.stdout).group(1))
                self.assertAlmostEqual(angle,expected)
                self.assertNotIn('creg c[0]',result.stdout)

    def test_barrier_and_identity_do_not_drop_operations(self):
        result=self.export('call void @__quantum__qis__id(%Qubit* %q)\ncall void @__quantum__qis__barrier(%Qubit* %q)\nret void')
        self.assertEqual(result.returncode,0,result.stderr)
        self.assertIn('barrier q[0];',result.stdout)

    def test_unknown_dynamic_and_nonfinite_parameters_fail_without_output(self):
        for body, message in [
            ('call void @__quantum__qis__unknown(%Qubit* %q)\nret void','Unsupported QIS'),
            ('call void @__quantum__qis__rx(double %parameter, %Qubit* %q)\nret void','Unresolved symbolic'),
            ('call void @__quantum__qis__rx(double 0x7FF0000000000000, %Qubit* %q)\nret void','Non-finite'),
            ('%angle = fdiv double 1.0, 0.0\ncall void @__quantum__qis__rx(double %angle, %Qubit* %q)\nret void','Division by zero'),
            ('call void @__quantum__qis__rx(double 0.1, %Qubit* null)\nret void','unresolved quantum operand'),
            ('br i1 %flag, label %yes, label %no\nyes:\ncall void @__quantum__qis__id(%Qubit* %q)\nbr label %no\nno:\nret void','Unsupported classical control-flow')]:
            with self.subTest(message=message):
                result=self.export(body)
                self.assertEqual(result.returncode,1,result.stderr)
                self.assertIn(message,result.stderr)
                self.assertFalse(result.stdout)


if __name__=='__main__':
    unittest.main()
