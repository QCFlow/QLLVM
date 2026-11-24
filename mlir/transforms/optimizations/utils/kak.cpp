/*
 * This code is part of QLLVM.
 *
 * (C) Copyright QCFlow 2026.
 *
 * This code is licensed under the Apache License, Version 2.0. You may
 * obtain a copy of this license in the LICENSE file in the root directory
 * of this source tree or at https://www.apache.org/licenses/LICENSE-2.0.
 *
 * Any modifications or derivative works of this code must retain this
 * copyright notice, and modified files need to carry a notice indicating
 * that they have been altered from the originals.
 */
// Modified 2026-11-22: exact Weyl reconstruction, explicit tensor order and numerical guards.
#include "get_matrix.hpp"
#include "local_unitary.hpp"
#include "synthesis_diagnostics.hpp"
#include "weyl_diagonalization.hpp"
#include <Eigen/Dense>
#include <complex>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <cmath>
#include <random>
#include "kak.hpp"
#include "gate_matrix.hpp"

#define _USE_MATH_DEFINES
constexpr double TOLERANCE = 1e-15;
constexpr double INPUT_UNITARY_TOLERANCE = 1e-10;
constexpr double WEYL_DIAGONALIZATION_TOLERANCE = 1e-12;
std::unordered_set<std::string> basic_gate_set;

using namespace Eigen;
using namespace std;
namespace {

const Eigen::Matrix4cd _B_nonnormalized = (Eigen::Matrix4cd() <<
    complex<double>(1, 0), complex<double>(0, 1), complex<double>(0, 0), complex<double>(0, 0),
    complex<double>(0, 0), complex<double>(0, 0), complex<double>(0, 1), complex<double>(1, 0),
    complex<double>(0, 0), complex<double>(0, 0), complex<double>(0, 1), complex<double>(-1, 0),
    complex<double>(1, 0), complex<double>(0, -1), complex<double>(0, 0), complex<double>(0, 0)
).finished();

const Eigen::Matrix4cd _B_nonnormalized_dagger = 0.5 * _B_nonnormalized.adjoint();

Eigen::Matrix4cd transform_to_magic_basis(const Eigen::Matrix4cd& U, bool reverse = false) {
    if (reverse) {
        return _B_nonnormalized_dagger * U * _B_nonnormalized;
    } else {
        return _B_nonnormalized * U * _B_nonnormalized_dagger;
    }
}

double trace_to_fid(const std::complex<double>& trace) {
    return (4 + std::norm(trace)) / 20;
}

bool is_close(double a, double b, double c, double ap, double bp, double cp, double fidelity=((1.0 - 1.0e-6))) {
    // calculate the angle difference
    double da = a - ap;
    double db = b - bp;
    double dc = c - cp;

    // calculate the complex number tr
    std::complex<double> tr(
        cos(da) * cos(db) * cos(dc),
        sin(da) * sin(db) * sin(dc)
    );

    // The trigonometric product is the trace divided by four. Qiskit's
    // average-gate fidelity uses the full trace, whose maximum is 4.
    double fid = trace_to_fid(std::complex<double>(4.0, 0.0) * tr);

    // check if it is greater than or equal to the threshold
    return fid >= fidelity;
}

Eigen::MatrixXcd& ip_x() {
  static Eigen::MatrixXcd matrix = [] {
      Eigen::MatrixXcd m(2, 2);
      m << 0.0, 1i, 1i, 0.0;
      return m;
  }();
  return matrix;
}
Eigen::MatrixXcd& ip_y() {
  static Eigen::MatrixXcd matrix = [] {
      Eigen::MatrixXcd m(2, 2);
      m << 0.0, 1, -1, 0.0;
      return m;
  }();
  return matrix;
}
Eigen::MatrixXcd& ip_z(){
  static Eigen::MatrixXcd matrix = [] {
      Eigen::MatrixXcd m(2, 2);
      m << 1i, 0.0, 0.0, -1i;
      return m;
  }();
  return matrix;
}
Eigen::MatrixXd& i_d(){
  static Eigen::MatrixXd matrix = [] {
      Eigen::MatrixXd m(2, 2);
      m << 1, 0.0, 0.0, 1;
      return m;
  }();
  return matrix;
}
Eigen::MatrixXcd rz_mat(double &in_params) {
  auto theta = in_params;
  Eigen::Matrix2cd result = Eigen::MatrixXcd::Zero(2, 2);
  result << std::exp(std::complex<double>(0, -0.5 * theta)), 0.0, 0.0,
      std::exp(std::complex<double>(0, 0.5*theta));
  return result;
}
Eigen::MatrixXcd ry_mat(double &in_params) {
  auto theta = in_params;
  Eigen::Matrix2cd result = Eigen::MatrixXcd::Zero(2, 2);
  result << std::cos(0.5 * theta), -std::sin(0.5 * theta),
      std::sin(0.5 * theta), std::cos(0.5 * theta);
  return result;
}
Eigen::MatrixXcd rx_mat(double &in_params) {
  auto theta = in_params;
  Eigen::Matrix2cd result = Eigen::MatrixXcd::Zero(2, 2);
  result << std::cos(0.5 * theta),
      std::complex<double>(0, -1) * std::sin(0.5 * theta),
      std::complex<double>(0, -1) * std::sin(0.5 * theta),
      std::cos(0.5 * theta);
  return result;
}

struct DecompositionResult {
  Eigen::Matrix2cd L;
  Eigen::Matrix2cd R;
  double phase;
};

DecompositionResult decompose_two_qubit_product_gate(const Eigen::Matrix4cd& special_unitary_matrix) {
  
  Matrix2cd R = special_unitary_matrix.topLeftCorner(2, 2);
  auto detR = R.determinant();
  if (std::abs(detR) < 0.1) {
    R = special_unitary_matrix.bottomLeftCorner(2, 2);
    detR = R.determinant();
  }

  if (abs(detR) < 0.1) {
    // std::cout << "decompose_two_qubit_product_gate: unable to decompose: detR < 0.1" << std::endl;
    Eigen::Matrix2cd L = Eigen::MatrixXcd::Zero(2, 2);
    Eigen::Matrix2cd R = Eigen::MatrixXcd::Zero(2, 2);
    double phase = 0.0;
    DecompositionResult result;
    result.L = L;
    result.R = R;
    result.phase = phase;
    return result;
  }
  R /= std::sqrt(detR);

  Eigen::Matrix4cd temp = kroneckerProduct(Eigen::Matrix2cd::Identity(),R.adjoint());
  temp = special_unitary_matrix * temp;

  // Extract L
  Eigen::Matrix2cd L;
  L << temp(0, 0), temp(0, 2),temp(2, 0), temp(2, 2);
  auto detL = L.determinant();

  if (abs(detL) < 0.9) {
    // std::cout << "decompose_two_qubit_product_gate: unable to decompose: detR < 0.9" << std::endl;
    Eigen::Matrix2cd L = Eigen::MatrixXcd::Zero(2, 2);
    Eigen::Matrix2cd R = Eigen::MatrixXcd::Zero(2, 2);
    double phase = 0.0;
    DecompositionResult result;
    result.L = L;
    result.R = R;
    result.phase = phase;
    return result;
  }
  L /= std::sqrt(detL);

  // Calculate phase
  double phase = std::arg(detL) / 2.0;

  // Verify decomposition
  Eigen::Matrix4cd combined = kroneckerProduct(L,R);

  std::complex<double> trace_val = (combined.adjoint() * special_unitary_matrix).trace();
  double deviation = abs(abs(trace_val) - 4.0);
  if (deviation > 1.0e-9) {
    // std::cout << "decompose_two_qubit_product_gate: decomposition failed: deviation too large" << std::endl;
    Eigen::Matrix2cd L = Eigen::MatrixXcd::Zero(2, 2);
    Eigen::Matrix2cd R = Eigen::MatrixXcd::Zero(2, 2);
    double phase = 0.0;
    DecompositionResult result;
    result.L = L;
    result.R = R;
    result.phase = phase;
    return result;
  }
  DecompositionResult result;
  result.L = L;
  result.R = R;
  result.phase = phase;
  return result;
}

double closest_partial_swap(double a, double b, double c) {
    double m = (a + b + c) / 3.0;
    double am = a - m, bm = b - m, cm = c - m;
    double ab = a - b, bc = b - c, ca = c - a;
    
    return m + am * bm * cm * (6.0 + ab*ab + bc*bc + ca*ca) / 18.0;
}
// If the matrix is finite: no NaN elements
template <typename Derived>
inline bool isFinite(const Eigen::MatrixBase<Derived> &x) {
  return ((x - x).array() == (x - x).array()).all();
}

// Default tolerace for validation
bool allClose(const Eigen::MatrixXcd &in_mat1, const Eigen::MatrixXcd &in_mat2,
              double in_tol = TOLERANCE) {
  // std::cout << "in_mat1\n"<< in_mat1 << std::endl;              
  if (!isFinite(in_mat1) || !isFinite(in_mat2)) {
    return false;
  }

  if (in_mat1.rows() == in_mat2.rows() && in_mat1.cols() == in_mat2.cols()) {
    for (int i = 0; i < in_mat1.rows(); ++i) {
      for (int j = 0; j < in_mat1.cols(); ++j) {
        if (std::abs(in_mat1(i, j) - in_mat2(i, j)) > in_tol) {
          return false;
        }
      }
    }

    return true;
  }
  return false;
}

struct decompose_site {
  Eigen::Matrix2cd L1;
  Eigen::Matrix2cd L2;
  Eigen::Matrix2cd R1;
  Eigen::Matrix2cd R2;
  double a;
  double b;
  double c;
  double g;
};

Eigen::Matrix4cd compute_exp_diagonal_matrix(const Eigen::Vector4d& d) {
  MatrixXcd D = MatrixXcd::Zero(4,4);

  for(int i = 0; i < 4; ++i) {
      // calculate exp(i * d_i) = cos(d_i) + i * sin(d_i)
      std::complex<double> val = std::polar(1.0, d(i)); // polar(r,theta)
      D(i, i) = val;
  }
  return D;
}

decompose_site weyl_decomposition(const Eigen::Matrix4cd& unitary_matrix) {
  
  Eigen::Matrix4cd U = unitary_matrix;
  for(int i = 0;i < 4;i++){
    for(int j = 0;j < 4;j++){
      // Compare the magnitude. A signed comparison zeros every negative entry
      // and makes the Weyl decomposition fail on ordinary rotations.
      if(std::abs(U(i,j).real()) < TOLERANCE){
        U(i,j).real(0.0);
      }
      if(std::abs(U(i,j).imag()) < TOLERANCE){
        U(i,j).imag(0.0);
      }
    }
  }
  complex<double> detU = U.determinant();
  
  if(abs(detU.real()) < TOLERANCE){
    detU.real(0.0);
  }
  if(abs(detU.imag()) < TOLERANCE){
    detU.imag(0.0);
  }

  U *= pow(detU, -0.25);
  double global_phase = arg(detU) / 4.0;
  
  Eigen::Matrix4cd Up = transform_to_magic_basis(U, true);
  Eigen::Matrix4cd M2 = Up.transpose() * Up;
  
  for(int i = 0; i < 4;i++){
    for(int j = 0;j < 4;j++){
      if(abs(M2(i,j).real()) < TOLERANCE){
        M2(i,j).real(0.0);
      }
      if(abs(M2(i,j).imag()) < TOLERANCE){
        M2(i,j).imag(0.0);
      }
    }
  }

  Eigen::Matrix4d P;
  Eigen::Vector4cd D;
  // Modified 2026-11-21: common scalar weights preserve the simultaneous
  // eigenbasis; elementwise random weights caused unnecessary failed retries.
  const bool diagonalized =
      qllvm::kak::diagonalizeSymmetricUnitary(M2, P, D, WEYL_DIAGONALIZATION_TOLERANCE);
  
  if(!diagonalized) {
    // std::cout << "TwoQubitWeylDecomposition: failed to diagonalize M2" << std::endl;
    decompose_site result;
    result.L1 = Eigen::Matrix2cd::Zero();
    result.L2 = Eigen::Matrix2cd::Zero();
    result.R1 = Eigen::Matrix2cd::Zero();
    result.R2 = Eigen::Matrix2cd::Zero();
    result.a = 0.0;
    result.b = 0.0;
    result.c = 0.0;
    result.g = 0.0;
    return result;
  }

  // 4. calculate the d and cs parameters
  Eigen::Vector4d d;
  for (int i = 0; i < 4; ++i) {
      d(i) = -arg(D(i)) / 2.0;
  }
  d(3) = -d(0) - d(1) - d(2);
  
  Eigen::Vector3d cs;
  for (int i = 0; i < 3; ++i) {
    cs(i) = fmod((d(i) + d(3)) / 2.0, 2 * M_PI);
    if(cs(i) < 0){
      cs(i) += 2 * M_PI;
    }
  }

  Eigen::Vector3d cstemp;
  for (int i = 0; i < 3; ++i) {
      cstemp(i) = fmod(cs(i), M_PI_2);
      cstemp(i) = min(cstemp(i), M_PI_2 - cstemp(i));
  }

  // get the sorted index
  vector<int> indices = {0,1,2};
  sort(indices.begin(), indices.end(), [&cstemp](int a, int b) {
      return cstemp(a) < cstemp(b);
  });
  
  // Arrange sorted coordinates as middle/largest/smallest before the
  // Weyl reflections. Sorting {1,2,0} itself loses this permutation.
  indices = {indices[1], indices[2], indices[0]};

  // apply the sorted index
  Eigen::Vector3d cs_ordered = cs;
  Eigen::Vector4d d_ordered = d;
  Eigen::Matrix4d P_reordered = P;

  for (int i = 0; i < 3; ++i) {
    cs_ordered(i) = cs(indices[i]);
    d_ordered(i) = d(indices[i]);
    P_reordered.col(i) = P.col(indices[i]);
  }

  P = P_reordered;
  d = d_ordered;
  cs = cs_ordered;
  // 6. adjust the determinant sign of P to make it in SO(4)
  if (P.determinant() < 0) {
    P.col(3) = -P.col(3);
  }

  auto K1 = transform_to_magic_basis(Up * P * compute_exp_diagonal_matrix(d));
  auto K2 = transform_to_magic_basis(P.transpose());

  for(int i = 0; i < 4;i++){
    for(int j = 0;j < 4;j++){
      if(abs(K1(i,j).real()) < TOLERANCE){
        K1(i,j).real(0.0);
      }
      if(abs(K1(i,j).imag()) < TOLERANCE){
        K1(i,j).imag(0.0);
      }
    }
  }
  for(int i = 0; i < 4;i++){
    for(int j = 0;j < 4;j++){
      if(abs(K2(i,j).real()) < TOLERANCE){
        K2(i,j).real(0.0);
      }
      if(abs(K2(i,j).imag()) < TOLERANCE){
        K2(i,j).imag(0.0);
      }
    }
  }
  

  // 3. decompose K1 and K2 into tensor product form
  DecompositionResult result_1 = decompose_two_qubit_product_gate(K1);
  auto K1l = result_1.L;
  auto K1r = result_1.R;
  auto phase_l = result_1.phase;
  if(K1l == Eigen::Matrix2cd::Zero() && K1r== Eigen::Matrix2cd::Zero() && phase_l==0.0){
    decompose_site result;
    result.L1 = Eigen::Matrix2cd::Zero();
    result.L2 = Eigen::Matrix2cd::Zero();
    result.R1 = Eigen::Matrix2cd::Zero();
    result.R2 = Eigen::Matrix2cd::Zero();
    result.a = 0.0;
    result.b = 0.0;
    result.c = 0.0;
    result.g = 0.0;
    return result;
  }

  DecompositionResult result_2 = decompose_two_qubit_product_gate(K2);
  auto K2l = result_2.L;
  auto K2r = result_2.R;
  auto phase_r = result_2.phase;
  if(K2l == Eigen::Matrix2cd::Zero() && K2r== Eigen::Matrix2cd::Zero() && phase_r == 0.0){
    decompose_site result;
    result.L1 = Eigen::Matrix2cd::Zero();
    result.L2 = Eigen::Matrix2cd::Zero();
    result.R1 = Eigen::Matrix2cd::Zero();
    result.R2 = Eigen::Matrix2cd::Zero();
    result.a = 0.0;
    result.b = 0.0;
    result.c = 0.0;
    result.g = 0.0;
    return result;
  }
  global_phase += phase_l + phase_r;


  if(cs(0) > M_PI_2){

      cs(0) -= 3 * M_PI_2;
      K1l = K1l* ip_y();
      K1r = K1r* ip_y();
      global_phase += M_PI_2;
  }
  if(cs(1)> M_PI_2){

      cs[1] -= 3 * M_PI_2;
      K1l = K1l* ip_x();
      K1r = K1r* ip_x();
      global_phase += M_PI_2;
  }
  auto conjs = 0;
  if (cs(0)> M_PI_4){

      cs(0) = M_PI_2 - cs(0);
      K1l = K1l* ip_y();
      K2r = ip_y() * K2r;
      conjs += 1;
      global_phase -= M_PI_2;
  }

  if (cs(1)> M_PI_4){

    cs(1) = M_PI_2 - cs(1);
    K1l = K1l* ip_x();
    K2r = ip_x() * K2r;
    conjs += 1;
    global_phase += M_PI_2;
    if(conjs == 1){
      global_phase -= M_PI;
    }
          
  }        
  if (cs(2)> M_PI_2){
    cs(2) -= 3 * M_PI_2;
    K1l = K1l* ip_z();
    K1r = K1r * ip_z();
    global_phase += M_PI_2;
    if(conjs == 1){
      global_phase -= M_PI;
    }
  }    

  if(conjs == 1){

      cs(2) = M_PI_2 - cs(2);
      K1l = K1l*ip_z();
      K2r = ip_z()*K2r;
      global_phase += M_PI_2;
  }
      
  if (cs(2)> M_PI_4){

      cs(2) -= M_PI_2;
      K1l = K1l* ip_z();
      K1r = K1r * ip_z();
      global_phase -= M_PI_2;
  }
  auto a = cs[1];
  auto b = cs[0];
  auto c = cs[2];
  // Keep the exact Weyl coordinates. Specializing near-degenerate cases by
  // average fidelity can exceed the final operator residual and must not
  // silently approximate a block outside the circuit's error budget.

  decompose_site result;
  result.L1 = K1l;
  result.L2 = K1r;
  result.R1 = K2l;
  result.R2 = K2r;
  result.a = a;
  result.b = b;
  result.c = c;
  result.g = global_phase;

  return result;
}

inline bool isSquare(const Eigen::MatrixXcd &in_mat) {
  return in_mat.rows() == in_mat.cols();
}

bool isUnitary(const Eigen::MatrixXcd &in_mat) {
  if (!isSquare(in_mat) || !isFinite(in_mat)) {
    return false;
  }

  Eigen::MatrixXcd Id =
      Eigen::MatrixXcd::Identity(in_mat.rows(), in_mat.cols());

  return allClose(in_mat * in_mat.adjoint(), Id, INPUT_UNITARY_TOLERANCE);
}

// Compute exp(i(x XX + y YY + z ZZ)) matrix
Eigen::Matrix4cd interactionMatrixExp(double x, double y, double z) {
  Eigen::MatrixXcd X{Eigen::MatrixXcd::Zero(2, 2)};
  Eigen::MatrixXcd Y{Eigen::MatrixXcd::Zero(2, 2)};
  Eigen::MatrixXcd Z{Eigen::MatrixXcd::Zero(2, 2)};
  X << 0, 1, 1, 0;
  Y << 0, -1i, 1i, 0;
  Z << 1, 0, 0, -1;
  auto XX = Eigen::kroneckerProduct(X, X);
  auto YY = Eigen::kroneckerProduct(Y, Y);
  auto ZZ = Eigen::kroneckerProduct(Z, Z);
  Eigen::MatrixXcd herm = x * XX + y * YY + z * ZZ;
  herm = 1i * herm;
  Eigen::MatrixXcd unitary = herm.exp();
  return unitary;
}

std::vector<qllvm::kak::TMP_OP>
singleQubitGateGen(Eigen::Matrix2cd in_mat, size_t in_bitIdx,std::unordered_set<std::string> basic_gate_set){
  using GateMatrix = Eigen::Matrix2cd;
  std::vector<qllvm::utils::EulerBasis> target_basis_set;
  std::vector<std::string> basis_names = {};
  if(basic_gate_set.size() > 0){
    basis_names.clear();
    if(basic_gate_set.find("rz") != basic_gate_set.end() && basic_gate_set.find("ry") != basic_gate_set.end()){
        basis_names.emplace_back("ZYZ");
    }
    if(basic_gate_set.find("rx") != basic_gate_set.end() && basic_gate_set.find("ry") != basic_gate_set.end()){
        basis_names.emplace_back("XYX");
    }
    if(basic_gate_set.find("rz") != basic_gate_set.end() && basic_gate_set.find("rx") != basic_gate_set.end()){
        basis_names.emplace_back("ZXZ");
        basis_names.emplace_back("XZX");
    }
    if(basic_gate_set.find("sx") != basic_gate_set.end() && basic_gate_set.find("x") != basic_gate_set.end()){
        basis_names.emplace_back("ZSXX");
    }
    if(basic_gate_set.find("rz") != basic_gate_set.end() && basic_gate_set.find("sx") != basic_gate_set.end()){
        basis_names.emplace_back("ZSX");
    }
  }
  for(int i = 0;i < basis_names.size();i++){
    auto basis = qllvm::utils::euler_Basis_FromStr(basis_names[i]);
    target_basis_set.emplace_back(basis);
  }
  auto simplified_seq = qllvm::utils::leastcost_basis(in_mat,target_basis_set);
  std::vector<qllvm::kak::TMP_OP> result;
  for(auto i = 0; i < simplified_seq.size();i++){
    auto temp = qllvm::kak::TMP_OP(simplified_seq[i].first, simplified_seq[i].second, {in_bitIdx});
    result.emplace_back(temp);
  }

  return result;
}

} // namespace

namespace qllvm {
namespace kak {
void mergeVectors(std::vector<qllvm::kak::TMP_OP> &vec1, std::vector<qllvm::kak::TMP_OP> &vec2) {
    vec1.insert(vec1.end(), vec2.begin(), vec2.end());
}

KAK::SynthesisResult KAK::expand(Eigen::Matrix4cd unitary, std::vector<size_t> bits,std::unordered_set<std::string>  basic_gate_set) {
  if (!isUnitary(unitary)) {
    if (synthesis::allowDiagnostic()) std::cerr << "expand: input not unitary\n";
    return std::nullopt;
  }
  // Modified 2026-11-24: recognize only a bit-exact scalar identity here.
  // No tolerance or approximation budget is spent by this fast path.
  bool identity = true;
  for (int row = 0; row < 4; ++row)
    for (int col = 0; col < 4; ++col)
      identity &= unitary(row, col) == (row == col ? unitary(0, 0) : std::complex<double>(0, 0));
  if (identity) return std::vector<TMP_OP>{};
  auto result = kakDecomposition(unitary);
  if (!result.has_value()) {
    if (synthesis::allowDiagnostic()) std::cerr << "expand: kak decomposition failed\n";
    return std::nullopt;
  }
  
  if (!qllvm::matrix::sameLocalUnitary(unitary, result->toMat())) {
    // The candidate permutations below only explain a rejected decomposition.
    // Retain rejection and its original checks even when diagnostics are off.
    if (synthesis::allowDiagnostic()) {
      auto aligned = [](const Eigen::MatrixXcd &reference,
                        const Eigen::MatrixXcd &candidate) {
        const std::complex<double> overlap =
            (reference.adjoint() * candidate).trace();
        if (std::abs(overlap) < 1e-15) return candidate.norm();
        const std::complex<double> phase = overlap / std::abs(overlap);
        return (candidate - phase * reference).norm();
      };
      const double angles[6][3] = {
          {result->x, result->y, result->z}, {result->x, result->z, result->y},
          {result->y, result->x, result->z}, {result->y, result->z, result->x},
          {result->z, result->x, result->y}, {result->z, result->y, result->x}};
      double best = 1e9;
      int bestCase = -1;
      for (int order = 0; order < 2; ++order) {
        auto before = order == 0 ? Eigen::kroneckerProduct(result->b1, result->b0)
                                 : Eigen::kroneckerProduct(result->b0, result->b1);
        auto after = order == 0 ? Eigen::kroneckerProduct(result->a1, result->a0)
                                : Eigen::kroneckerProduct(result->a0, result->a1);
        for (int perm = 0; perm < 6; ++perm) {
          auto candidate = result->g * after *
                           interactionMatrixExp(angles[perm][0], angles[perm][1],
                                                angles[perm][2]) *
                           before;
          double distance = aligned(unitary, candidate);
          if (distance < best) {
            best = distance;
            bestCase = order * 6 + perm;
          }
        }
      }
      std::cerr << "expand: reconstructed matrix mismatch x=" << result->x
                << " y=" << result->y << " z=" << result->z
                << " best=" << best << " case=" << bestCase << "\n";
    }
    return {};
  }
  auto gates = result->toGates(bits[0], bits[1],basic_gate_set);
  try {
    Eigen::MatrixXcd candidate = Eigen::MatrixXcd::Identity(4, 4);
    for (const auto &gate : gates) {
      std::vector<size_t> axes;
      for (int operand : gate.indexs) {
        auto it = std::find(bits.begin(), bits.end(), size_t(operand));
        if (it == bits.end()) {
          if (synthesis::allowDiagnostic()) std::cerr << "expand: operand outside the block\n";
          return {};
        }
        axes.push_back(std::distance(bits.begin(), it));
      }
      auto local = qllvm::matrix::getGateMat({gate.name, gate.params}, 0);
      candidate = qllvm::matrix::embedLocalUnitary(local, axes, 2) * candidate;
    }
    if (!qllvm::matrix::sameLocalUnitary(unitary, candidate)) {
      if (synthesis::allowDiagnostic()) std::cerr << "expand: gate sequence mismatch gates=" << gates.size()
                << " x=" << result->x << " y=" << result->y
                << " z=" << result->z << "\n";
      return {};
    }
  } catch (const std::exception &error) {
    if (synthesis::allowDiagnostic()) std::cerr << "expand: " << error.what() << "\n";
    return {};
  }
  return gates;
}

std::optional<KAK::KakDecomposition>
KAK::kakDecomposition(const InputMatrix& in_matrix) const {
  auto wel_site = weyl_decomposition(in_matrix);

  auto a0 = wel_site.L2;
  auto a1 = wel_site.L1;
  auto b0 = wel_site.R2;
  auto b1 = wel_site.R1;
  auto x = wel_site.a;
  auto y = wel_site.b;
  auto z = wel_site.c;
  auto g = wel_site.g;

  if(a0 == Eigen::Matrix2cd::Zero() && a1 == Eigen::Matrix2cd::Zero() && b0 == Eigen::Matrix2cd::Zero() && b1 == Eigen::Matrix2cd::Zero()
      && x == 0.0 && y == 0.0 && z == 0.0 && g == 0.0){
    // KakDecomposition result;
    // return result;
    return std::nullopt;
  }

  KakDecomposition result;
  {
    result.a0 = a0;
    result.a1 = a1;
    result.b0 = b0;
    result.b1 = b1;
    result.x = x;
    result.y = y;
    result.z = z;
    result.g = std::polar(1.0, g);
  }
  return result;
}

Eigen::MatrixXcd KAK::KakDecomposition::toMat() const {
  auto before = Eigen::kroneckerProduct(b1, b0);
  auto after = Eigen::kroneckerProduct(a1, a0);
  Eigen::MatrixXcd unitary = interactionMatrixExp(x, y, z);
  auto total = g * after * unitary * before;
  return total;
}

std::vector<qllvm::kak::TMP_OP>
KAK::KakDecomposition::toGates(size_t in_bit1, size_t in_bit2,std::unordered_set<std::string>  basic_gate_set) const {

  const auto generateInteractionComposite = [&](size_t bit1, size_t bit2, double x, double y, double z) {
    const double TOL = 1e-12;
    // A vanishing interaction is local gates only. Emitting two CZ plus a
    // zero rotation here is what turned cancelling pairs back into entanglers.
    if (std::abs(x) < TOL && std::abs(y) < TOL && std::abs(z) < TOL)
      return std::vector<qllvm::kak::TMP_OP>{};
    // Modified 2026-11-22: the Weyl point (pi/4,0,0) is the one-CZ
    // class. H.H conjugates ZZ into XX; local -pi/2 Z rotations turn
    // CZ into exp(i*pi/4*ZZ), up to global phase. Other Ising angles
    // still need two CZs. The complete emitted sequence is validated below.
    if (std::abs(x - M_PI_4) < TOL && std::abs(y) < TOL && std::abs(z) < TOL) {
      return std::vector<qllvm::kak::TMP_OP>{
          {"h", {}, {int(bit1)}}, {"h", {}, {int(bit2)}},
          {"rz", {-M_PI_2}, {int(bit1)}}, {"rz", {-M_PI_2}, {int(bit2)}},
          {"cz", {}, {int(bit1), int(bit2)}},
          {"h", {}, {int(bit1)}}, {"h", {}, {int(bit2)}}};
    }
    // Full decomposition is required
    if (std::abs(z) >= TOL) {
      const double xAngle = M_PI * (x * -2 / M_PI + 0.5);
      const double yAngle = M_PI * (y * -2 / M_PI + 0.5);
      const double zAngle = M_PI * (z * -2 / M_PI + 0.5);
      std::vector<qllvm::kak::TMP_OP> composite;
      auto h = qllvm::kak::TMP_OP("h", {}, {bit1});
      auto cz = qllvm::kak::TMP_OP("cz", {}, {bit2, bit1});
      auto h1 = qllvm::kak::TMP_OP("h", {}, {bit1});
      auto rz = qllvm::kak::TMP_OP("rz", {zAngle}, {bit1});
      auto rx = qllvm::kak::TMP_OP("rx", {M_PI_2}, {bit1});
      auto h2 = qllvm::kak::TMP_OP("h", {}, {bit2});
      auto cz1 = qllvm::kak::TMP_OP("cz", {}, {bit1, bit2});
      auto h3 = qllvm::kak::TMP_OP("h", {}, {bit2});
      auto ry = qllvm::kak::TMP_OP("ry", {yAngle}, {bit1});
      auto rx1 = qllvm::kak::TMP_OP("rx", {xAngle}, {bit2});
      auto h4 = qllvm::kak::TMP_OP("h", {}, {bit1});
      auto cz2 = qllvm::kak::TMP_OP("cz", {}, {bit1, bit2});
      auto h5 = qllvm::kak::TMP_OP("h", {}, {bit1});
      auto rx2 = qllvm::kak::TMP_OP("rx", {-M_PI_2}, {bit2});

      composite.emplace_back(h);
      composite.emplace_back(cz);
      composite.emplace_back(h1);
      composite.emplace_back(rz);
      composite.emplace_back(rx);
      composite.emplace_back(h2);
      composite.emplace_back(cz1);
      composite.emplace_back(h3);
      composite.emplace_back(ry);
      composite.emplace_back(rx1);
      composite.emplace_back(h4);
      composite.emplace_back(cz2);
      composite.emplace_back(h5);
      composite.emplace_back(rx2);
      return composite;
    }else if (y >= TOL) {
      const double xAngle = -2 * x;
      const double yAngle = -2 * y;
      std::vector<qllvm::kak::TMP_OP> composite;
      auto rx = qllvm::kak::TMP_OP("rx", {M_PI_2}, {bit2});
      auto h = qllvm::kak::TMP_OP("h", {}, {bit1});
      auto cz = qllvm::kak::TMP_OP("cz", {}, {bit2, bit1});
      auto h1 = qllvm::kak::TMP_OP("h", {}, {bit1});
      auto ry = qllvm::kak::TMP_OP("ry", {yAngle}, {bit1});
      auto rx1 = qllvm::kak::TMP_OP("rx", {xAngle}, {bit2});
      auto h2 = qllvm::kak::TMP_OP("h", {}, {bit1});
      auto cz1 = qllvm::kak::TMP_OP("cz", {}, {bit1, bit2});
      auto h3 = qllvm::kak::TMP_OP("h", {}, {bit1});
      auto rx2 = qllvm::kak::TMP_OP("rx", {-M_PI_2}, {bit2});

      composite.emplace_back(rx);
      composite.emplace_back(h);
      composite.emplace_back(cz);
      composite.emplace_back(h1);
      composite.emplace_back(ry);
      composite.emplace_back(rx1);
      composite.emplace_back(h2);
      composite.emplace_back(cz1);
      composite.emplace_back(h3);
      composite.emplace_back(rx2);
      
      
      return composite;
    }else {// only XX is significant
      const double xAngle = -2 * x;
      std::vector<qllvm::kak::TMP_OP> composite;
      auto h = qllvm::kak::TMP_OP("h", {}, {bit1});
      auto cz = qllvm::kak::TMP_OP("cz", {}, {bit2, bit1});
      auto rx = qllvm::kak::TMP_OP("rx", {xAngle}, {bit2});
      auto cz1 = qllvm::kak::TMP_OP("cz", {}, {bit1, bit2});
      auto h1 = qllvm::kak::TMP_OP("h", {}, {bit1});

      composite.emplace_back(h);
      composite.emplace_back(cz);
      composite.emplace_back(rx);
      composite.emplace_back(cz1);
      composite.emplace_back(h1);
      
      return composite;
    }
  };

  auto a0Comp = singleQubitGateGen(a0, in_bit1,basic_gate_set);
  auto a1Comp = singleQubitGateGen(a1, in_bit2,basic_gate_set);
  auto b0Comp = singleQubitGateGen(b0, in_bit1,basic_gate_set);
  auto b1Comp = singleQubitGateGen(b1, in_bit2,basic_gate_set);

  auto interactionComp =
      generateInteractionComposite(in_bit1, in_bit2, x, y, z);

  std::vector<qllvm::kak::TMP_OP> totalComposite;
  qllvm::kak::mergeVectors(totalComposite, b0Comp);
  qllvm::kak::mergeVectors(totalComposite, b1Comp);
  qllvm::kak::mergeVectors(totalComposite, interactionComp);
  qllvm::kak::mergeVectors(totalComposite, a0Comp);
  qllvm::kak::mergeVectors(totalComposite, a1Comp);
  
  // Ignore global phase
  return totalComposite;
}

} // namespace kak
} // namespace qllvm
