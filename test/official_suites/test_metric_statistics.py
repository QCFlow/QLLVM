from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / 'benchmark/official_suites'))
from metric_statistics import two_qubit_outcomes


class MetricStatisticsTests(unittest.TestCase):
    def pair(self, left, right, suite='a', basis=True):
        return dict(qllvm_two_qubit_gates=left, peer_two_qubit_gates=right,
                    suite=suite, common_basis=basis)

    def test_zero_pairs_have_outcomes_without_entering_logarithmic_mean(self):
        pairs = [self.pair(0, 3), self.pair(0, 0), self.pair(3, 0),
                 self.pair(2, 3), self.pair(3, 3), self.pair(4, 3)]
        result = two_qubit_outcomes(pairs)
        self.assertEqual(result, dict(compared=6, better=2, equal=2, worse=2,
                                     zero_better=1, zero_equal=1, zero_worse=1, positive_pairs=3))

    def test_suite_and_common_basis_filter_preserved(self):
        result = two_qubit_outcomes([self.pair(0, 3), self.pair(0, 4, suite='b'),
                                     self.pair(0, 5, basis=False)], 'a')
        self.assertEqual(result['compared'], 1)
        self.assertEqual(result['zero_better'], 1)

    def test_invalid_negative_gate_count_rejected(self):
        with self.assertRaises(ValueError):
            two_qubit_outcomes([self.pair(-1, 3)])
