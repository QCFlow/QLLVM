/* QLLVM correctness revision, QCFlow 2026. Apache-2.0.
 * Explicit budget for intentional near-identity deletions. Numerical errors in
 * other passes are checked separately; this is not a universal compiler proof.
 */
#pragma once
#include <cmath>
#include <cstddef>
#include <limits>
#include <stdexcept>

namespace qllvm {
namespace approximation {
struct Budget {
  double limit = 1e-6;
  double spent = 0;
  size_t deletions = 0;
  size_t retainedByBudget = 0;
};
inline thread_local Budget current;
inline void reset(double limit) {
  if (!std::isfinite(limit) || limit < 0 || limit > 2)
    throw std::runtime_error("Near-identity budget must be finite and between 0 and 2");
  current = Budget{limit, 0, 0, 0};
}
inline double rotationDistanceBound(double theta) {
  if (!std::isfinite(theta))
    throw std::runtime_error("Non-finite near-identity angle");
  // Full channel diamond distance for a one-qubit rotation/phase deletion.
  // An exactly zero angle is exact. Account conservatively for FP evaluation
  // of nonzero angles; the tiny guard is not the pass's approximation threshold.
  double delta = theta == 0 ? 0 : std::min(2.0,
      2 * std::abs(std::sin(theta / 2)) + 8 * std::numeric_limits<double>::epsilon());
  return delta == 0 ? 0 : std::nextafter(delta, std::numeric_limits<double>::infinity());
}
inline bool chargeRotation(double theta) {
  const double delta = rotationDistanceBound(theta);
  const double next = delta == 0 ? current.spent :
      std::nextafter(current.spent + delta, std::numeric_limits<double>::infinity());
  if (next > current.limit) {
    ++current.retainedByBudget;
    return false;
  }
  current.spent = next;
  ++current.deletions;
  return true;
}
} // namespace approximation
} // namespace qllvm
