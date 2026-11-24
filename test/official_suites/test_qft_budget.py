"""Prioritize tiny rotations without changing the cumulative error contract."""
import json
import os
from pathlib import Path
import subprocess
import tempfile
import unittest

import numpy as np
from qiskit import QuantumCircuit, qasm2
from qiskit.quantum_info import Operator


@unittest.skipUnless(os.environ.get('QLLVM_TEST_COMPILER'), 'Set QLLVM_TEST_COMPILER')
class QFTBudgetTests(unittest.TestCase):
    def test_parity_phase_gadget_deletion_spends_one_certified_budget(self):
        c=QuantumCircuit(2)
        c.cx(0,1);c.rz(8e-7,1);c.cx(0,1)
        compiled,budget=self.compile(c)
        self.assertEqual(sum(i.operation.num_qubits==2 for i in compiled.data),0)
        self.assertGreater(budget['charged_upper_bound'],0)
        self.assert_bound(c,compiled,budget)
        exact,strict=self.compile(c,exact=True)
        self.assertEqual(sum(i.operation.num_qubits==2 for i in exact.data),2)
        self.assertEqual(strict['charged_upper_bound'],0)

    def test_canonical_phase_and_overlapping_gadgets_keep_the_budget(self):
        for u3 in (False,True):
            c=QuantumCircuit(3)
            for k in range(16):
                c.cx(k%2,2)
                if u3:c.u(0,0,1e-8,2)
                else:c.rz(1e-8,2)
                c.cx(k%2,2)
            compiled,budget=self.compile(c)
            self.assertEqual(sum(i.operation.num_qubits==2 for i in compiled.data),0)
            self.assert_bound(c,compiled,budget)

    def compile(self, circuit, exact=False, custom=False):
        with tempfile.TemporaryDirectory() as directory:
            d = Path(directory)
            (d/'input.qasm').write_text(qasm2.dumps(circuit))
            command = [os.environ['QLLVM_TEST_COMPILER'], str(d/'input.qasm'), '-O1',
                       '-mlir-disable-threading', '-emit-backend=qasm-backend',
                       '-basicgate=[rx,ry,rz,cz,h,x,y,z]', '-output-path='+str(d/'output.qasm'),
                       '-approx-report='+str(d/'budget.json')]
            if exact:
                command.append('-exact-rotations')
            if custom:
                (d/'passes.txt').write_text('0\n')
                command.append('-customPassSequence='+str(d/'passes.txt'))
            done = subprocess.run(command, capture_output=True, text=True, timeout=60)
            self.assertEqual(done.returncode, 0, done.stdout+done.stderr)
            result = qasm2.load(str(d/'output.qasm'), custom_instructions=qasm2.LEGACY_CUSTOM_INSTRUCTIONS)
            return result, json.loads((d/'budget.json').read_text())

    def assert_bound(self, original, compiled, budget):
        self.assertLessEqual(budget['charged_upper_bound'], budget['limit'])
        a, b = Operator(original).data, Operator(compiled).data
        phase = np.vdot(a, b); phase /= abs(phase)
        error = np.linalg.norm(b - phase*a, ord=2)
        self.assertLessEqual(error, budget['charged_upper_bound'] + 1e-9)

    def test_tiny_late_rotations_get_budget_before_larger_early_rotation(self):
        c = QuantumCircuit(2)
        c.rx(9e-7, 0)
        for _ in range(64):
            c.rz(1e-8, 1)
        compiled, budget = self.compile(c, custom=True)
        self.assertEqual(compiled.count_ops().get('rz', 0), 0)
        self.assertEqual(compiled.count_ops().get('rx', 0), 1)
        self.assertEqual(budget['accepted_deletions'], 64)
        self.assert_bound(c, compiled, budget)
        exact, exact_budget = self.compile(c, exact=True, custom=True)
        self.assertEqual(exact.count_ops().get('rz', 0), 64)
        self.assertEqual(exact_budget['charged_upper_bound'], 0)
        self.assertTrue(Operator(c).equiv(Operator(exact), atol=1e-10, rtol=0))

    def test_qft_small_angles_and_full_optimization_keep_error_bound(self):
        c = QuantumCircuit(4)
        c.rx(8e-7, 0)
        for k in range(12):
            c.h(k % 4)
            c.cp(1e-8/(k+1), k % 4, (k+1) % 4)
            c.cx((k+2) % 4, (k+3) % 4)
        compiled, budget = self.compile(c)
        self.assertEqual(budget['limit'], 1e-6)
        self.assert_bound(c, compiled, budget)
        exact, exact_budget = self.compile(c, exact=True)
        self.assertEqual(exact_budget['charged_upper_bound'], 0)
        self.assertTrue(Operator(c).equiv(Operator(exact), atol=1e-7, rtol=0))
