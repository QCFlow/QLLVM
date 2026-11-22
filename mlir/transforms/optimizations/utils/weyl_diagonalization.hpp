// (C) Copyright QCFlow 2026. Licensed under Apache-2.0.
// Modified 2026-11-21: preserve the common eigenvectors of Re(M) and Im(M).
#pragma once
#include <Eigen/Dense>
#include <random>

namespace qllvm {
namespace kak {
inline bool diagonalizeSymmetricUnitary(const Eigen::Matrix4cd &matrix,
                                       Eigen::Matrix4d &vectors,
                                       Eigen::Vector4cd &diagonal,
                                       double tolerance,
                                       unsigned *attemptCount = nullptr) {
  if (attemptCount) *attemptCount = 0;
  if (!matrix.allFinite() || !std::isfinite(tolerance) || tolerance < 0)
    return false;
  std::mt19937_64 rng(2020);
  std::normal_distribution<double> normal(0.0, 1.0);
  for (unsigned attempt = 0; attempt < 100; ++attempt) {
    // A common pair of scalar weights preserves simultaneous eigenvectors.
    // Independent element weights destroy that property and symmetry.
    const double realWeight = normal(rng), imagWeight = normal(rng);
    const Eigen::Matrix4d real =
        realWeight * matrix.real() + imagWeight * matrix.imag();
    Eigen::SelfAdjointEigenSolver<Eigen::Matrix4d> solver(real);
    if (attemptCount) ++*attemptCount;
    if (solver.info() != Eigen::Success) continue;
    vectors = solver.eigenvectors();
    diagonal = (vectors.transpose() * matrix * vectors).diagonal();
    const Eigen::Matrix4cd reconstructed =
        vectors * diagonal.asDiagonal() * vectors.transpose();
    // Same elementwise absolute residual as the original allClose() guard.
    // No tolerance changes; failure leaves the caller's original fallback.
    if (reconstructed.allFinite() &&
        (reconstructed - matrix).cwiseAbs().maxCoeff() <= tolerance)
      return true;
  }
  return false;
}
} // namespace kak
} // namespace qllvm
