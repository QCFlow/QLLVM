"""Deterministic repairs for malformed static QASM2 benchmark inputs."""
from pathlib import Path
import sys
import tempfile
import unittest
import hashlib
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2] / "benchmark/official_suites"
sys.path.insert(0, str(ROOT))
import worker
import input_repairs


class InputNormalizationTests(unittest.TestCase):
    def test_reviewed_restoration_requires_both_content_hashes(self):
        body = 'OPENQASM 2.0;\nqreg q0[3];\nqreg q1[6];\nqreg q2[1];\n'
        suffix = ''.join('measure q[{}] -> c[{}];\n'.format(i, i) for i in range(6))
        source = body + suffix
        key = hashlib.sha256(source.encode()).hexdigest()
        blob = hashlib.sha1(b'blob ' + str(len(body)).encode() + b'\0' + body.encode()).hexdigest()
        records = {key: {'file': 'fixture.qasm', 'revision': 'fixture', 'blob': blob}}
        with patch.dict(input_repairs.HHL_RESTORATIONS, records, clear=True):
            restored, changes = worker.normalize_qasm2_input(source)
            self.assertEqual(restored, body)
            self.assertIn(blob, changes[0])
            with self.assertRaisesRegex(ValueError, 'Ambiguous undeclared'):
                worker.normalize_qasm2_input(source.replace('q0[3]', 'q0[4]'))
            records[key]['blob'] = '0' * 40
            with self.assertRaisesRegex(ValueError, 'prior official Git blob'):
                worker.normalize_qasm2_input(source)

    def test_archived_official_inputs_match_documented_repairs(self):
        base = ROOT / 'upstream/qasmbench'
        if not (base/'small/hhl_n10/hhl_n10.qasm').exists():
            self.skipTest('Official archive is not installed')
        for source_hash, record in input_repairs.HHL_RESTORATIONS.items():
            raw = (base/record['file']).read_bytes()
            self.assertEqual(hashlib.sha256(raw).hexdigest(), source_hash)
            fixed, changes = worker.normalize_qasm2_input(raw.decode())
            encoded = fixed.encode()
            self.assertEqual(hashlib.sha1(b'blob ' + str(len(encoded)).encode() + b'\0' + encoded).hexdigest(), record['blob'])
            self.assertNotIn('measure', fixed)
            self.assertEqual(len(changes), 1)
        source = (base/'large/vqe_uccsd_n28/vqe_uccsd_n28_transpiled.qasm').read_text()
        fixed, changes = worker.normalize_qasm2_input(source)
        self.assertEqual(fixed.replace('\ncreg c[28];', '', 1), source)
        self.assertEqual(changes, ['inserted creg c[28]'])

    def test_single_qreg_vqe_alias_is_repaired_without_changing_gate_body(self):
        source = '''OPENQASM 2.0;
include "qelib1.inc";
qreg reg[3];
h reg[0];
cx reg[0],reg[1];
rz(0.2) reg[2];
measure q[0] -> c[0];
measure q[1] -> c[1];
measure q[2] -> c[2];
'''
        normalized, changes = worker.normalize_qasm2_input(source)
        self.assertIn("creg c[3];", normalized)
        self.assertIn("measure reg[2] -> c[2];", normalized)
        self.assertEqual(changes, ["inserted creg c[3]", "mapped terminal q[i] measurements to reg[i]"])
        before = source.split("measure", 1)[0]
        after = normalized.split("measure", 1)[0]
        self.assertEqual(before, after.replace("creg c[3];\n", ""))

    def test_single_qreg_with_existing_creg_only_maps_alias(self):
        source = '''OPENQASM 2.0;
include "qelib1.inc";
qreg reg[2];
creg c[2];
x reg[1];
measure q[0] -> c[0];
measure q[1] -> c[1];
'''
        normalized, changes = worker.normalize_qasm2_input(source)
        self.assertNotIn("creg c[2];\ncreg", normalized)
        self.assertEqual(normalized.count("measure reg["), 2)
        self.assertEqual(changes, ["mapped terminal q[i] measurements to reg[i]"])

    def test_multiple_qregs_are_rejected_without_guessing(self):
        source = '''OPENQASM 2.0;
include "qelib1.inc";
qreg q0[1];
qreg q1[1];
measure q[0] -> c[0];
measure q[1] -> c[1];
'''
        with self.assertRaisesRegex(ValueError, "Ambiguous undeclared"):
            worker.normalize_qasm2_input(source)

    def test_partial_or_nonterminal_alias_is_rejected(self):
        partial = '''OPENQASM 2.0;
include "qelib1.inc";
qreg reg[2];
measure q[0] -> c[0];
'''
        with self.assertRaisesRegex(ValueError, "identity or partial"):
            worker.normalize_qasm2_input(partial)
        nonterminal = partial.replace("measure q[0] -> c[0];", "x q[0];\nmeasure q[0] -> c[0];\nmeasure q[1] -> c[1];")
        with self.assertRaisesRegex(ValueError, "outside terminal"):
            worker.normalize_qasm2_input(nonterminal)

    def test_gate_after_complete_measurement_map_is_rejected(self):
        source = 'OPENQASM 2.0; qreg reg[1]; measure q[0] -> c[0]; x reg[0];'
        with self.assertRaisesRegex(ValueError, "terminal"):
            worker.normalize_qasm2_input(source)

    def test_existing_q_name_only_inserts_missing_classical_register(self):
        source = 'OPENQASM 2.0; qreg q[1]; h q[0]; measure q[0] -> c[0];'
        normalized, changes = worker.normalize_qasm2_input(source)
        self.assertEqual(changes, ['inserted creg c[1]'])
        self.assertEqual(normalized, source.replace('qreg q[1];', 'qreg q[1];\ncreg c[1];'))

    def test_comments_cannot_supply_mapping_or_be_rewritten(self):
        comment = '// qreg fake[99]; measure q[0] -> c[0];\n'
        source = 'OPENQASM 2.0;\n' + comment + 'qreg reg[1];\nh reg[0];\nmeasure q[0] -> c[0]; // tail\n'
        normalized, changes = worker.normalize_qasm2_input(source)
        self.assertIn(comment, normalized)
        self.assertIn('measure reg[0] -> c[0]; // tail', normalized)
        self.assertEqual(len(changes), 2)
        fake = 'OPENQASM 2.0; qreg reg[1];\n' + comment
        self.assertEqual(worker.normalize_qasm2_input(fake), (fake, []))

    def test_conflicting_classical_width_or_register_is_rejected(self):
        for declarations in ('creg c[2];', 'creg other[1];',
                             'creg c[1]; creg other[1];'):
            source = 'OPENQASM 2.0; qreg reg[1]; ' + declarations + ' measure q[0] -> c[0];'
            with self.subTest(declarations=declarations):
                with self.assertRaisesRegex(ValueError, 'matching width'):
                    worker.normalize_qasm2_input(source)

    def test_classical_feedback_is_rejected(self):
        source = 'OPENQASM 2.0; qreg reg[1]; creg c[1]; if(c==0) measure q[0] -> c[0];'
        with self.assertRaisesRegex(ValueError, 'static terminal'):
            worker.normalize_qasm2_input(source)


if __name__ == "__main__":
    unittest.main()
