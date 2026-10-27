"""SDK adapter regressions: preserve wire identities, reject dynamic erasure."""
import importlib.util
from pathlib import Path
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2] / "benchmark"
sys.path.insert(0, str(ROOT / "official_suites"))
import worker

import verification as checker


class SDKAdapterTests(unittest.TestCase):
    def paths(self, directory, text):
        source, output = Path(directory)/"input.qasm", Path(directory)/"output.qasm"
        source.write_text(text)
        return source, output

    def assert_equivalent(self, source, output):
        result = checker.verify(worker.load_qasm(source), worker.load_qasm(output), 6, 1e-7, "coherent")
        self.assertEqual(result["status"], "passed", result)

    def test_pyqpanda3_single_qubit_does_not_require_nonempty_topology(self):
        with tempfile.TemporaryDirectory() as directory:
            source, output = self.paths(directory, 'OPENQASM 2.0;\ninclude "qelib1.inc";\nqreg q[1];\ncreg c[1];\nu3(0.2,0.3,0.7) q[0];\nmeasure q[0] -> c[0];\n')
            worker.pyqpanda3_compile(source, output)
            self.assert_equivalent(source, output)

    def test_pennylane_preserves_partial_permuted_measurements(self):
        with tempfile.TemporaryDirectory() as directory:
            source, output = self.paths(directory, 'OPENQASM 2.0;\ninclude "qelib1.inc";\nqreg q[3];\ncreg c[3];\nu3(0.3,0.2,0.8) q[0];\ncx q[0],q[2];\nmeasure q[0] -> c[2];\nmeasure q[2] -> c[0];\n')
            worker.pennylane_compile(source, output)
            self.assert_equivalent(source, output)

    def test_pennylane_rejects_mid_circuit_measurement(self):
        with tempfile.TemporaryDirectory() as directory:
            source, output = self.paths(directory, 'OPENQASM 2.0;\ninclude "qelib1.inc";\nqreg q[1];\ncreg c[1];\nmeasure q[0] -> c[0];\nu3(0.2,0,0) q[0];\n')
            with self.assertRaises(NotImplementedError):
                worker.pennylane_compile(source, output)

    def test_pennylane_rejects_repeated_classical_destinations(self):
        with tempfile.TemporaryDirectory() as directory:
            source, output = self.paths(directory, 'OPENQASM 2.0;\ninclude "qelib1.inc";\nqreg q[2];\ncreg c[1];\nmeasure q[0] -> c[0];\nmeasure q[1] -> c[0];\n')
            with self.assertRaises(NotImplementedError):
                worker.pennylane_compile(source, output)

    def test_serialization_preserves_unitary_and_measurement_map(self):
        with tempfile.TemporaryDirectory() as directory:
            source, output = self.paths(directory, 'OPENQASM 2.0;\ninclude "qelib1.inc";\nqreg q[3];\ncreg c[3];\nh q[1];\nccx q[0],q[1],q[2];\nmeasure q[2] -> c[1];\n')
            worker.prepare(source, output)
            self.assert_equivalent(source, output)


if __name__ == "__main__":
    unittest.main()
