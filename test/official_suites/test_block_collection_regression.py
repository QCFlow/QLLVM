"""Semantic tests for indexed two-wire block collection."""
import os
from pathlib import Path
import random
import re
import subprocess
import tempfile
import unittest
from unittest.mock import patch
from qiskit import QuantumCircuit, qasm2
from qiskit.quantum_info import Operator
import test_qllvm_parser_regression as parser_tests

@unittest.skipUnless(os.environ.get('QLLVM_TEST_COMPILER'), 'Set QLLVM_TEST_COMPILER')
class BlockCollectionTests(unittest.TestCase):
    def test_compilation_cache_reuses_only_exact_results_across_rounds(self):
        c=QuantumCircuit(4)
        for k in range(18):
            for a,b in ((0,1),(2,3)):
                c.cx(a,b);c.rx(.37 if k%2 else .370000000001,a)
                c.rz(-.23,b);c.cx(b,a)
        outputs=[]
        for disabled in ('0','1'):
            with patch.dict(os.environ,QLLVM_DISABLE_SYNTHESIS_CACHE=disabled):
                result,text=parser_tests.ParserRegressionTests.compile(self,qasm2.dumps(c),'O1',45,
                    ['-mlir-disable-threading','-trace-timings','-exact-rotations'])
            self.assertEqual(result.returncode,0,result.stdout+result.stderr)
            after=qasm2.loads(text,custom_instructions=qasm2.LEGACY_CUSTOM_INSTRUCTIONS)
            self.assertTrue(Operator(c).equiv(Operator(after),atol=1e-7,rtol=0))
            outputs.append(text)
            m=re.search(r'\[qllvm-synthesis-cache\] hits=(\d+) misses=(\d+) bytes=(\d+)',result.stderr)
            self.assertIsNotNone(m,result.stderr)
            if disabled=='0':self.assertGreater(int(m[1]),0)
            else:self.assertEqual(tuple(map(int,m.groups())),(0,0,0))
        self.assertEqual(outputs[0],outputs[1])

    def test_collection_state_is_not_written_to_the_ir(self):
        c=QuantumCircuit(3)
        for k in range(6):
            c.rx(.13*k,0);c.cx(0,1);c.rz(.23,1);c.cx(1,2)
        result,text=parser_tests.ParserRegressionTests.compile(self,qasm2.dumps(c),'O1',45,
            ['-mlir-disable-threading','-emitmlir'])
        self.assertEqual(result.returncode,0,result.stdout+result.stderr)
        self.assertIn('MLIR',result.stdout)
        self.assertIsNone(re.search(r'\b(?:selected|onebit|location)\s*=',result.stdout))
        after=qasm2.loads(text,custom_instructions=qasm2.LEGACY_CUSTOM_INSTRUCTIONS)
        self.assertTrue(Operator(c).equiv(Operator(after),atol=1e-7,rtol=0))

    def test_target_u3_lowering_does_not_depend_on_collection_metadata(self):
        c=QuantumCircuit(3)
        c.rx(.27,2);c.rz(-.37,2);c.cx(2,0);c.ry(.43,0);c.rx(-.21,0)
        with tempfile.TemporaryDirectory() as tmp:
            source,output=Path(tmp)/'in.qasm',Path(tmp)/'out.qasm'
            source.write_text(qasm2.dumps(c))
            done=subprocess.run([os.environ['QLLVM_TEST_COMPILER'],str(source),'-O1',
                '-mlir-disable-threading','-emit-backend=qasm-backend','-basicgate=[u3,cz]',
                '-output-path='+str(output)],capture_output=True,text=True,timeout=45)
            self.assertEqual(done.returncode,0,done.stdout+done.stderr)
            after=qasm2.load(str(output),custom_instructions=qasm2.LEGACY_CUSTOM_INSTRUCTIONS)
        self.assertTrue(Operator(c).equiv(Operator(after),atol=1e-7,rtol=0))
        self.assertLessEqual(set(after.count_ops()),{'u3','cz'})

    def test_identity_block_removes_both_outputs_without_numerical_synthesis(self):
        c=QuantumCircuit(4)
        c.h(2)
        c.cz(1,0);c.cz(1,0)
        c.cx(2,3)
        with tempfile.TemporaryDirectory() as tmp:
            passes=Path(tmp)/'passes.txt';passes.write_text('1\n')
            result,text=parser_tests.ParserRegressionTests.compile(self,qasm2.dumps(c),'O1',45,
                ['-customPassSequence='+str(passes),'-mlir-disable-threading','-exact-rotations'])
        self.assertEqual(result.returncode,0,result.stdout+result.stderr)
        after=qasm2.loads(text,custom_instructions=qasm2.LEGACY_CUSTOM_INSTRUCTIONS)
        self.assertTrue(Operator(c).equiv(Operator(after),atol=1e-9,rtol=0))
        self.assertEqual(after.count_ops().get('cz',0),1)
        self.assertEqual(len(after.data),4)  # Independent H + target-basis CX.

    def test_synthesis_diagnostics_are_opt_in_and_preserve_the_operator(self):
        c=QuantumCircuit(2)
        for k in range(180):
            c.cx(0,1);c.rx(.2+k*.001,0);c.rz(-.31,1);c.cx(1,0)
        outputs=[]
        for enabled in ['0','1']:
            with patch.dict(os.environ,QLLVM_SYNTHESIS_DIAGNOSTICS=enabled):
                result,output=parser_tests.ParserRegressionTests.compile(self,qasm2.dumps(c),'O1',45,
                    ['-mlir-disable-threading'])
            self.assertEqual(result.returncode,0,result.stdout+result.stderr)
            diagnostics=[line for line in (result.stdout+result.stderr).splitlines()
                         if line.startswith(('expand:','empty block=','pair n=','kak block='))]
            self.assertLessEqual(len(diagnostics),256)
            if enabled=='0':self.assertEqual(diagnostics,[])
            else:self.assertTrue(diagnostics)
            outputs.append(qasm2.loads(output,custom_instructions=qasm2.LEGACY_CUSTOM_INSTRUCTIONS))
        for after in outputs:
            self.assertTrue(Operator(c).equiv(Operator(after),atol=1e-7,rtol=0))

    def check(self, circuit, sequence='1\n1\n'):
        with tempfile.TemporaryDirectory() as directory:
            passes = Path(directory)/'passes.txt'
            passes.write_text(sequence)
            result, output = parser_tests.ParserRegressionTests.compile(self, qasm2.dumps(circuit),
                'O1', 45, ['-customPassSequence='+str(passes), '-mlir-disable-threading'])
        self.assertEqual(result.returncode, 0, result.stdout+result.stderr)
        after=qasm2.loads(output,custom_instructions=qasm2.LEGACY_CUSTOM_INSTRUCTIONS)
        self.assertTrue(Operator(circuit).equiv(Operator(after),atol=1e-7,rtol=0))

    def test_interleaved_disjoint_pairs_and_reversed_operands(self):
        c=QuantumCircuit(6)
        for k in range(16):
            for a,b in [(0,1),(3,2),(4,5)]:
                c.cx(a,b);c.rz(.03*(k+1),a);c.rx(.17,b)
        self.check(c)

    def test_overlapping_pair_stops_at_third_wire(self):
        c=QuantumCircuit(4)
        for _ in range(12):
            c.cx(0,1);c.rx(.23,1);c.cx(1,2);c.rz(.17,0)
            c.cx(0,1);c.h(3);c.cx(2,3);c.cx(1,0)
        self.check(c)

    def test_multiple_rewrites_preserve_random_circuits(self):
        for seed in range(8):
            with self.subTest(seed=seed):
                rng=random.Random(seed);c=QuantumCircuit(4)
                for _ in range(60):
                    a,b=rng.sample(range(4),2)
                    c.u(rng.random(),rng.random(),rng.random(),a)
                    c.cx(a,b)
                self.check(c)

    def test_dense_two_qubit_blocks_and_degenerate_interactions(self):
        # Different/nearby angles, reversed controls, and exactly cancelling
        # entanglers exercise synthesis acceptance as well as safe fallback.
        for seed in range(16):
            with self.subTest(seed=seed):
                rng=random.Random(917+seed);c=QuantumCircuit(2)
                for k in range(20):
                    a,b=(0,1) if k%2 else (1,0)
                    for q in [a,b]:c.u(rng.uniform(-3,3),rng.uniform(-3,3),rng.uniform(-3,3),q)
                    c.cx(a,b)
                    if seed%4==0:c.cx(a,b)
                    c.rz(.37 if k%2 else .370000000001,a)
                self.check(c,sequence='1\n1\n6\n')

    def test_repeated_euler_synthesis_keeps_exact_angles_and_wire_order(self):
        c=QuantumCircuit(4)
        for k in range(120):
            a,b=k%4,(k+1)%4
            angle=0.37 if k%3 else 0.370000000001
            c.rx(angle,a);c.rz(-.23,a);c.ry(.18,a)
            c.cx(a,b)
            c.ry(.18,b);c.rz(-.23,b);c.rx(-angle,b)
        self.check(c,sequence='1\n6\n1\n6\n')
