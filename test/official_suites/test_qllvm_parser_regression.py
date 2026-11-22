"""Compiler regressions for expression prediction and long flat QASM inputs.

Run in the benchmark SDK environment with QLLVM_TEST_COMPILER pointing at the
binary under test. No suite downloads or live benchmark modifications required.
"""
import os
from pathlib import Path
import subprocess
import tempfile
import unittest
import json
import sys
import re

from qiskit import QuantumCircuit, qasm2
from qiskit.quantum_info import Operator


COMPILER = os.environ.get("QLLVM_TEST_COMPILER")
HEADER = 'OPENQASM 2.0;\ninclude "qelib1.inc";\n'


@unittest.skipUnless(COMPILER, "Set QLLVM_TEST_COMPILER to the compiler under test")
class ParserRegressionTests(unittest.TestCase):
    def test_whitespace_comments_multiline_gates_and_frontend_timing(self):
        text=HEADER+'''// u3(9e10) and OPENQASM in comments stay comments.
    gate custom(theta) a {
        rx(theta) a;
        rz(-theta / 2) a;
    }
\tqreg q[2];
    custom(3E-2) q[0]; /* a multi-line comment
        cx q[0],q[1]; */
\t u3(-4e-1, 5e+0, 2E-3) q[1];
    cx q[1],q[0];'''
        reference=re.sub(r'/\*.*?\*/','',text,flags=re.S)
        original=qasm2.loads(reference,custom_instructions=qasm2.LEGACY_CUSTOM_INSTRUCTIONS)
        for extra in [[],['-trace-timings']]:
            result,out=self.compile(text,'O1',30,extra)
            self.assertEqual(result.returncode,0,result.stdout+result.stderr)
            after=qasm2.loads(out,custom_instructions=qasm2.LEGACY_CUSTOM_INSTRUCTIONS)
            self.assertTrue(Operator(original).equiv(Operator(after),atol=1e-7,rtol=0))
            for phase in ['preprocess','tokenize','parse','mlir-generate']:
                if extra:self.assertIn('phase='+phase,result.stderr)
                else:self.assertNotIn('[qllvm-frontend]',result.stderr)

    def compile(self, text, opt="O0", timeout=30, extra_args=()):
        with tempfile.TemporaryDirectory() as tmp:
            source, output = Path(tmp)/"input.qasm", Path(tmp)/"output.qasm"
            source.write_text(text)
            result = subprocess.run(
                [COMPILER, str(source), "-"+opt, "-emit-backend=qasm-backend",
                 "-output-path="+str(output), "-basicgate=[rx,ry,rz,cz,h,x,y,z]",
                 *extra_args],
                text=True, capture_output=True, timeout=timeout)
            return result, output.read_text() if output.exists() else None

    def assert_unitary(self, text, opt="O0"):
        result, output = self.compile(text, opt)
        self.assertEqual(result.returncode, 0, result.stdout+result.stderr)
        before = qasm2.loads(text, custom_instructions=qasm2.LEGACY_CUSTOM_INSTRUCTIONS)
        after = qasm2.loads(output, custom_instructions=qasm2.LEGACY_CUSTOM_INSTRUCTIONS)
        self.assertTrue(Operator(before).equiv(Operator(after), atol=1e-7, rtol=0), output)

    def test_expression_precedence_and_unary(self):
        text = HEADER + '''qreg q[2];
u3(pi/2+0.1, -(pi/3), pi/4*2) q[0];
cx q[0],q[1];
u3((pi/2-0.2)*0.5, sin(pi/4), cos(pi/7)) q[1];
rz(-pi/3/2) q[0];
rx(2*(pi/7+0.2)) q[1];
'''
        for opt in ("O0", "O1"):
            with self.subTest(opt=opt): self.assert_unitary(text, opt)

    def test_custom_gate_and_nested_expression(self):
        self.assert_unitary(HEADER + '''gate local(a,b) x { rz(a/2) x; rx(-(b+pi/3)) x; }
qreg q[2];
local(pi/4,0.7) q[1];
cx q[1],q[0];
''')

    def test_measurement_destinations(self):
        text = HEADER + '''qreg left[1]; qreg right[1]; creg c[2];
u3(pi/2,0,pi) right[0]; cx right[0],left[0];
measure left[0] -> c[1]; measure right[0] -> c[0];
'''
        result, output = self.compile(text)
        self.assertEqual(result.returncode, 0, result.stdout+result.stderr)
        def mapping(circuit):
            return [(circuit.find_bit(i.qubits[0]).index,
                     circuit.find_bit(i.clbits[0]).index)
                    for i in circuit.data if i.operation.name=="measure"]
        self.assertEqual(sorted(mapping(qasm2.loads(text))), sorted(mapping(qasm2.loads(output))))

    def test_invalid_inputs_are_rejected(self):
        for body in ["qreg q[1]; u3(pi/,0,0) q[0];",
                     "qreg q[1]; h q[0]; }",
                     "qreg q[1]; h q[0]; `",
                     "qreg q[1]; creg c[1]; measure missing[0] -> c[0];"]:
            with self.subTest(body=body):
                result, output = self.compile(HEADER+body)
                self.assertNotEqual(result.returncode, 0)
                self.assertFalse(output)

    def test_large_flat_circuit_preserves_every_operation(self):
        # Large enough to exercise the former repeated LL prediction hotspot.
        circuit = QuantumCircuit(36)
        for k in range(1400):
            circuit.u(0.3, 0.2, 0.7, k % 36)
            circuit.cx(k % 36, (k+1) % 36)
        text = qasm2.dumps(circuit)
        result, output = self.compile(text, timeout=30)
        self.assertEqual(result.returncode, 0, result.stdout+result.stderr)
        after = qasm2.loads(output, custom_instructions=qasm2.LEGACY_CUSTOM_INSTRUCTIONS)
        self.assertEqual(after.num_qubits, 36)
        self.assertEqual(after.count_ops().get('cz', 0), 1400)

    def test_deep_wire_chains_and_operand_order_after_rewrites(self):
        # Exercise cached wire discovery, both SSA result numbers, and repeated
        # optimization passes. Independent operator evaluation checks that no
        # stale cache survives a resynthesis or swaps control and target wires.
        circuit = QuantumCircuit(3)
        for k in range(450):
            circuit.u(0.03*(k % 7), 0.2, -0.17, k % 3)
            circuit.cx((k+1) % 3, k % 3)
        self.assert_unitary(qasm2.dumps(circuit), "O1")

    def test_commuting_groups_keep_wire_and_barrier_identity(self):
        circuit = QuantumCircuit(3)
        for k in range(40):
            a, b, c = k % 3, (k+1) % 3, (k+2) % 3
            circuit.cx(a, b)
            circuit.rz(0.19, a)
            circuit.cx(a, c)
            circuit.cx(a, b)
            circuit.h(c)  # separates groups on this wire
            circuit.cz(b, c)
            circuit.rx(-0.23, b)
            circuit.cz(b, c)
        self.assert_unitary(qasm2.dumps(circuit), "O1")

    def test_custom_pass_statistics_do_not_overflow_on_deep_wire(self):
        text = HEADER + 'qreg q[1];\n' + 'rx(0.123) q[0];\n'*40000
        with tempfile.TemporaryDirectory() as tmp:
            passes = Path(tmp)/'passes.txt'
            passes.write_text('0\n')  # identity removal, including depth stats
            result, output = self.compile(
                text, "O1", timeout=45,
                extra_args=['-customPassSequence='+str(passes)])
        self.assertEqual(result.returncode, 0, result.stdout+result.stderr)
        self.assertEqual(output.count('rx('), 40000)

    def test_many_deleted_and_merged_segments_keep_semantics(self):
        # Accumulating dead operations must not turn membership checks into
        # a quadratic scan, or revisit an operation after it has been merged.
        text = HEADER + 'qreg q[2];\n'
        text += ('rz(0) q[0]; rx(0.13) q[0]; rx(-0.13) q[0]; '
                 'cx q[0],q[1]; cx q[0],q[1];\n') * 4000
        with tempfile.TemporaryDirectory() as tmp:
            passes = Path(tmp)/'passes.txt'
            passes.write_text('0\n6\n4\n')
            result, output = self.compile(
                text, "O1", timeout=30,
                extra_args=['-customPassSequence='+str(passes)])
        self.assertEqual(result.returncode, 0, result.stdout+result.stderr)
        after = qasm2.loads(output, custom_instructions=qasm2.LEGACY_CUSTOM_INSTRUCTIONS)
        self.assertTrue(Operator(after).equiv(Operator(QuantumCircuit(2)),
                                            atol=1e-7, rtol=0))

    def test_timing_trace_is_opt_in_and_reports_completion(self):
        text = HEADER + 'qreg q[1]; h q[0];\n'
        plain, output = self.compile(text)
        traced, traced_output = self.compile(text, extra_args=['-trace-timings'])
        self.assertEqual(plain.returncode, 0)
        self.assertEqual(traced.returncode, 0)
        self.assertEqual(output, traced_output)
        self.assertNotIn('[qllvm-timing]', plain.stderr)
        for stage in ('frontend-start', 'frontend-end', 'optimization-end',
                      'backend-start', 'backend-end'):
            self.assertIn(stage, traced.stderr)

    def test_exponent_literals_and_identifier_boundaries(self):
        text = HEADER + '''// 12e3 and escaped quotes "1e9" must stay comments.
/* 7e-2 in a multiline
comment */
gate gate1e3(a) q { rx(a) q; }
qreg q[2];
gate1e3(1e-3) q[0];
u3(-2E-2, .3e+0, 4.0e-1) q[1];
cx q[0],q[1];
rz(1e-2+2e-2) q[0];
'''
        result, output = self.compile(text)
        self.assertEqual(result.returncode, 0, result.stdout+result.stderr)
        # Qiskit's QASM2 importer does not accept block comments; use an
        # independent circuit oracle without removing comments from our input.
        expected = QuantumCircuit(2)
        expected.rx(0.001, 0)
        expected.u(-0.02, 0.3, 0.4, 1)
        expected.cx(0, 1)
        expected.rz(0.03, 0)
        actual = qasm2.loads(output, custom_instructions=qasm2.LEGACY_CUSTOM_INSTRUCTIONS)
        self.assertTrue(Operator(expected).equiv(Operator(actual), atol=1e-7, rtol=0))

    def test_worker_records_single_thread_command_and_compile_checkpoint(self):
        worker = Path(__file__).resolve().parents[2]/'benchmark/official_suites/worker.py'
        with tempfile.TemporaryDirectory() as tmp:
            directory = Path(tmp)
            source = directory/'input.qasm'
            source.write_text(HEADER+'qreg q[1]; h q[0];\n')
            request = directory/'request.json'
            request.write_text(json.dumps(dict(framework='qllvm', compiler=COMPILER,
                directory=str(directory), input=str(source), opt='O0',
                verify_qubits=2, tolerance=1e-7)))
            process = subprocess.run([sys.executable, str(worker), '--request', str(request)],
                                     capture_output=True, text=True, timeout=30)
            self.assertEqual(process.returncode, 0, process.stderr)
            result = json.loads((directory/'worker.json').read_text())
            self.assertEqual(result['status'], 'verified', result)
            self.assertIn('-mlir-disable-threading', result['command'])
            checkpoint = json.loads((directory/'compile-checkpoint.json').read_text())
            self.assertEqual(checkpoint['status'], 'compiled')
            self.assertGreater(checkpoint['output_bytes'], 0)


if __name__ == "__main__":
    unittest.main()
