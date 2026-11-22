/*
 * QLLVM correctness revision, QCFlow 2026. Apache-2.0.
 * Added for operand-aware commutation and pre-commit resynthesis validation.
 */
#pragma once
#include <Eigen/Dense>
#include <algorithm>
#include <complex>
#include <stdexcept>
#include <vector>

namespace qllvm {
namespace matrix {
// Operand 0 is the least significant local bit, matching getGateMat(..., 0).
inline Eigen::MatrixXcd embedLocalUnitary(const Eigen::MatrixXcd &local,
                                        const std::vector<size_t> &axes,
                                        unsigned width) {
  if (width > 4 || axes.empty() || axes.size() > 2 ||
      local.rows() != (1 << axes.size()) || local.cols() != local.rows() ||
      !local.allFinite())
    throw std::runtime_error("Invalid local-unitary embedding");
  size_t mask = 0;
  for (size_t axis : axes) {
    if (axis >= width || (mask & (size_t(1) << axis)))
      throw std::runtime_error("Invalid or duplicate quantum operand");
    mask |= size_t(1) << axis;
  }
  const size_t dimension = size_t(1) << width;
  Eigen::MatrixXcd result = Eigen::MatrixXcd::Zero(dimension, dimension);
  for (size_t col = 0; col < dimension; ++col) {
    size_t localCol = 0;
    for (size_t j = 0; j < axes.size(); ++j)
      localCol |= ((col >> axes[j]) & 1) << j;
    for (size_t localRow = 0; localRow < size_t(local.rows()); ++localRow) {
      size_t row = col & ~mask;
      for (size_t j = 0; j < axes.size(); ++j)
        row |= ((localRow >> j) & 1) << axes[j];
      result(row, col) = local(localRow, localCol);
    }
  }
  return result;
}

// Frobenius residual bounds the spectral residual. This is a numerical guard,
// not a formal proof, and the caller's independent regression oracle is retained.
inline bool sameLocalUnitary(const Eigen::MatrixXcd &reference,
                             const Eigen::MatrixXcd &candidate,
                             double tolerance = 1e-9) {
  if (reference.rows() != candidate.rows() ||
      reference.cols() != candidate.cols() ||
      !reference.allFinite() || !candidate.allFinite())
    return false;
  const std::complex<double> overlap = (reference.adjoint() * candidate).trace();
  if (std::abs(overlap) < 1e-15)
    return false;
  const std::complex<double> phase = overlap / std::abs(overlap);
  return (candidate - phase * reference).norm() <= tolerance;
}
} // namespace matrix
} // namespace qllvm
