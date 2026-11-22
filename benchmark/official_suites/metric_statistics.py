"""Count all paired gate outcomes, including zero, outside logarithmic means."""


def two_qubit_outcomes(pairs, suite=None):
    counts = dict(compared=0, better=0, equal=0, worse=0,
                  zero_better=0, zero_equal=0, zero_worse=0, positive_pairs=0)
    for pair in pairs:
        if suite and pair['suite'] != suite:
            continue
        if not pair.get('common_basis'):
            continue
        left = pair['qllvm_two_qubit_gates']
        right = pair['peer_two_qubit_gates']
        if left is None or right is None:
            continue
        if left < 0 or right < 0:
            raise ValueError('Gate counts must be nonnegative')
        outcome = 'better' if left < right else 'equal' if left == right else 'worse'
        counts['compared'] += 1
        counts[outcome] += 1
        if left == 0 or right == 0:
            counts['zero_' + outcome] += 1
        else:
            counts['positive_pairs'] += 1
    return counts
