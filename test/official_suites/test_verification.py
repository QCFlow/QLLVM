"""Strict and terminal-classical verification boundary regressions."""
from pathlib import Path
import sys
import unittest
from qiskit import QuantumCircuit
sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "benchmark/official_suites"))
import verification as runner

class VerificationTests(unittest.TestCase):
    def test_terminal_phase_difference_requires_explicit_contract(self):
        a = QuantumCircuit(1, 1)
        a.h(0)
        a.measure(0, 0)
        b = QuantumCircuit(1, 1)
        b.h(0)
        b.z(0)
        b.measure(0, 0)
        self.assertEqual(runner.verify(a,b,6,1e-7)["status"],"failed")
        result = runner.verify(a,b,6,1e-7,"terminal-classical")
        self.assertEqual(result["status"],"passed")
        self.assertEqual(result["coherent_operator_diagnostic"]["status"],"failed")

    def test_classical_contract_rejects_missing_measurements(self):
        a = QuantumCircuit(1)
        self.assertEqual(runner.verify(a,a,6,1e-7,"terminal-classical")["status"],"unsupported")

    def test_classical_contract_detects_changed_distribution(self):
        a,b = QuantumCircuit(1,1),QuantumCircuit(1,1)
        a.h(0)
        a.measure(0,0)
        b.measure(0,0)
        self.assertEqual(runner.verify(a,b,6,1e-7,"terminal-classical")["status"],"failed")

    def test_classical_contract_detects_bit_destination_change(self):
        a,b = QuantumCircuit(2,2),QuantumCircuit(2,2)
        a.h(0)
        a.measure([0,1],[0,1])
        b.h(0)
        b.measure([0,1],[1,0])
        self.assertEqual(runner.verify(a,b,6,1e-7,"terminal-classical")["status"],"failed")

    def test_global_phase_allowed(self):
        a = QuantumCircuit(2)
        a.h(0)
        a.cx(0, 1)
        b = a.copy()
        b.global_phase = 0.4
        self.assertEqual(runner.verify(a, b, 6, 1e-7)["status"], "passed")

    def test_zero_state_agreement_does_not_prove_equivalence(self):
        a, b = QuantumCircuit(1), QuantumCircuit(1)
        b.z(0)  # Both produce |0>, but differ on superpositions.
        self.assertEqual(runner.verify(a, b, 6, 1e-7)["status"], "failed")

    def test_terminal_measurements_preserved(self):
        a = QuantumCircuit(2, 2)
        a.h(0)
        a.measure([0, 1], [0, 1])
        self.assertEqual(runner.verify(a, a.copy(), 6, 1e-7)["status"], "passed")
        b = QuantumCircuit(2, 2)
        b.h(0)
        b.measure([0, 1], [1, 0])
        self.assertEqual(runner.verify(a, b, 6, 1e-7)["status"], "failed")

    def test_intermediate_measurement_not_silently_removed(self):
        circuit = QuantumCircuit(1, 1)
        circuit.measure(0, 0)
        circuit.h(0)
        self.assertEqual(runner.verify(circuit, circuit, 6, 1e-7)["status"], "unsupported")

    def test_reset_not_silently_removed(self):
        circuit = QuantumCircuit(1)
        circuit.reset(0)
        self.assertEqual(runner.verify(circuit, circuit, 6, 1e-7)["status"], "unsupported")

    def test_large_circuit_is_not_a_semantic_pass(self):
        circuit = QuantumCircuit(7)
        self.assertEqual(runner.verify(circuit, circuit, 6, 1e-7)["status"], "not_checked")

if __name__ == "__main__":
    unittest.main()
