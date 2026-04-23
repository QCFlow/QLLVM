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
#include "get_matrix.hpp"
// Turn off Eigen warnings
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsuggest-override"
#pragma GCC diagnostic ignored "-Wdeprecated-copy"
#pragma GCC diagnostic ignored "-Wunused-function"
#include <Eigen/Dense>
#pragma GCC diagnostic pop
#include <cassert>
#include <cctype>
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include "op.hpp"
using namespace mlir;
namespace qllvm {
namespace matrix{
using namespace std::complex_literals;
std::vector<double> get_parameters(mlir::quantum::ValueSemanticsInstOp &op){
  auto qbits_op = op.getNumResults();
  std::vector<double> current_op_params;
  if(qbits_op == 1){
    if (op.getNumOperands() > 1) {
      for (size_t i = 1; i < op.getNumOperands(); ++i) {
        auto param = op.getOperand(i);
        // param.Value()
        assert(param.getType().isa<mlir::FloatType>());
        current_op_params.emplace_back(qllvm::OP::tryGetConstAngle(param));
      }
    }
  }else if(qbits_op == 2){
    if (op.getNumOperands() > 2) {
      for (size_t i = 2; i < op.getNumOperands(); ++i) {
        auto param = op.getOperand(i);
        // param.Value()
        assert(param.getType().isa<mlir::FloatType>());
        current_op_params.emplace_back(qllvm::OP::tryGetConstAngle(param));
      }
    }
  }
  return current_op_params;
}

bool ForwardOrReverse(std::vector<std::pair<std::string, int64_t>> bits,std::unordered_map<std::string, int> qbitseq){
  if(qbitseq.size() == 1 || bits.front().first == bits.back().first){
    if(bits.front().second < bits.back().second) return true;
    else return false;
  }else{
    if(qbitseq[bits.front().first] < qbitseq[bits.back().first]) return true;
    else return false;
  }
}

Eigen::MatrixXcd getGateMat(mlir::quantum::ValueSemanticsInstOp &op, std::unordered_map<std::string, int> qbitseq) {
  auto parameters = get_parameters(op);
  auto gate_name = op.name().str();
  std::pair<std::string, std::vector<double>> in_op = std::make_pair(gate_name,parameters);

  static const Eigen::Matrix2cd X_mat = []() {
    Eigen::Matrix2cd result = Eigen::MatrixXcd::Zero(2, 2);
    result << 0.0, 1.0, 1.0, 0.0;
    return result;
  }();
  static const Eigen::Matrix2cd Id_mat = []() {
    Eigen::Matrix2cd result = Eigen::MatrixXcd::Zero(2, 2);
    result << 1.0, 0.0, 0.0, 1.0;
    return result;
  }();
  static const Eigen::Matrix2cd Y_mat = []() {
    Eigen::Matrix2cd result = Eigen::MatrixXcd::Zero(2, 2);
    result << 0.0,-1j, 1j, 0.0;
    return result;
  }();
  static const Eigen::Matrix2cd Z_mat = []() {
    Eigen::Matrix2cd result = Eigen::MatrixXcd::Zero(2, 2);
    result << 1.0, 0.0, 0.0, -1.0;
    return result;
  }();
  static const Eigen::Matrix2cd H_mat = []() {
    Eigen::Matrix2cd result = Eigen::MatrixXcd::Zero(2, 2);
    result << M_SQRT1_2, M_SQRT1_2, M_SQRT1_2, -M_SQRT1_2;
    return result;
  }();
  static const Eigen::Matrix2cd SX_mat = []() {
    Eigen::Matrix2cd result = Eigen::MatrixXcd::Zero(2, 2);
    result << M_SQRT1_2, std::complex<double>(0,-1)*M_SQRT1_2, std::complex<double>(0,-1)*M_SQRT1_2, M_SQRT1_2;
    return result;
  }();
  static const Eigen::Matrix4cd CX_mat_0 = []() {
    Eigen::Matrix4cd result = Eigen::MatrixXcd::Zero(4,4);
    result << 1.0, 0.0, 0.0, 0.0,
              0.0, 0.0, 0.0, 1.0,
              0.0, 0.0, 1.0, 0.0,
              0.0, 1.0, 0.0, 0.0;
    return result;
  }();
  static const Eigen::Matrix4cd CX_mat_1 = []() {
    Eigen::Matrix4cd result = Eigen::MatrixXcd::Zero(4,4);
    result << 1.0, 0.0, 0.0, 0.0,
              0.0, 1.0, 0.0, 0.0,
              0.0, 0.0, 0.0, 1.0,
              0.0, 0.0, 1.0, 0.0;
    return result;
  }();
  static const Eigen::Matrix4cd CY_mat_0 = []() {
    Eigen::Matrix4cd result = Eigen::MatrixXcd::Zero(4,4);
    result << 1.0, 0.0, 0.0, 0.0,
              0.0, 0.0, 0.0, 1j,
              0.0, 0.0, 1.0, 0.0,
              0.0, -1j, 0.0, 0.0;
    return result;
  }();
  static const Eigen::Matrix4cd CY_mat_1 = []() {
    Eigen::Matrix4cd result = Eigen::MatrixXcd::Zero(4,4);
    result << 1.0, 0.0, 0.0, 0.0,
              0.0, 1.0, 0.0, 0.0,
              0.0, 0.0, 0.0, -1j,
              0.0, 0.0, 1j, 0.0;
    return result;
  }();
  static const Eigen::Matrix4cd CZ_mat = []() {
    Eigen::Matrix4cd result = Eigen::MatrixXcd::Zero(4,4);
    result << 1.0, 0.0, 0.0, 0.0,
              0.0, 1.0, 0.0, 0.0,
              0.0, 0.0, 1.0, 0.0,
              0.0, 0.0, 0.0, -1.0;
    return result;
  }();
  static const Eigen::Matrix4cd SWAP_mat = []() {
    Eigen::Matrix4cd result = Eigen::MatrixXcd::Zero(4,4);
    result << 1.0, 0.0, 0.0, 0.0,
              0.0, 0.0, 1.0, 0.0,
              0.0, 1.0, 0.0, 0.0,
              0.0, 0.0, 0.0, 1.0;
    return result;
  }();

  static const auto CPhase_mat = [](const std::vector<double> &in_params) {
    auto theta = in_params[0];
    Eigen::Matrix4cd result = Eigen::MatrixXcd::Zero(4, 4);
    result << 1.0, 0.0, 0.0, 0.0,
              0.0, 1.0, 0.0, 0.0,
              0.0, 0.0, 1.0, 0.0,
              0.0, 0.0, 0.0, std::exp(std::complex<double>(0,theta));
    return result;
  };

  static const auto rx_mat = [](const std::vector<double> &in_params) {
    assert(in_params.size() == 1);
    auto theta = in_params[0];
    Eigen::Matrix2cd result = Eigen::MatrixXcd::Zero(2, 2);
    result << std::cos(0.5 * theta),
        std::complex<double>(0, -1) * std::sin(0.5 * theta),
        std::complex<double>(0, -1) * std::sin(0.5 * theta),
        std::cos(0.5 * theta);
    return result;
  };

  static const auto ry_mat = [](const std::vector<double> &in_params) {
    assert(in_params.size() == 1);
    auto theta = in_params[0];
    Eigen::Matrix2cd result = Eigen::MatrixXcd::Zero(2, 2);
    result << std::cos(0.5 * theta), -std::sin(0.5 * theta),
        std::sin(0.5 * theta), std::cos(0.5 * theta);
    return result;
  };

  static const auto rz_mat = [](const std::vector<double> &in_params) {
    assert(in_params.size() == 1);
    auto theta = in_params[0];
    Eigen::Matrix2cd result = Eigen::MatrixXcd::Zero(2, 2);
    result << std::exp(std::complex<double>(0, -0.5 * theta)), 0.0, 0.0,
        std::exp(std::complex<double>(0, 0.5 * theta));
    return result;
  };

  static const auto p_mat = [](const std::vector<double> &in_params) {
    assert(in_params.size() == 1);
    auto theta = in_params[0];
    Eigen::Matrix2cd result = Eigen::MatrixXcd::Zero(2, 2);
    result << 1.0, 0.0, 0.0, std::exp(std::complex<double>(0, theta));
    return result;
  };

  static const auto u3_mat = [](const std::vector<double> &in_params) {
    assert(in_params.size() == 3);
    auto in_theta = in_params[0];
    auto in_phi = in_params[1];
    auto in_lambda = in_params[2];

    Eigen::Matrix2cd result = Eigen::MatrixXcd::Zero(2, 2);

    // qpp::cmat gateMat(2, 2);
    result << std::cos(in_theta / 2.0),
        -std::exp(std::complex<double>(0, in_lambda)) *
            std::sin(in_theta / 2.0),
        std::exp(std::complex<double>(0, in_phi)) * std::sin(in_theta / 2.0),
        std::exp(std::complex<double>(0, in_phi + in_lambda)) *
            std::cos(in_theta / 2.0);

    return result;
  };

  static const std::unordered_map<std::string, Eigen::Matrix2cd>
      GateMatrixCache = {{"x", X_mat},           {"y", Y_mat},{"id", Id_mat},
                        {"z", Z_mat},           {"h", H_mat},
                        {"t", p_mat({M_PI_4})}, {"tdg", p_mat({-M_PI_4})},
                        {"s", p_mat({M_PI_2})}, {"sdg", p_mat({-M_PI_2})},
                        {"sx", SX_mat}};
  const auto &gateName = in_op.first;
  const auto &gateParams = in_op.second;
  const auto it = GateMatrixCache.find(gateName);
  std::unordered_set<std::string> singleNoRotationGates = {"x", "y", "z", "h", "t", "tdg", "id", "s", "sdg", "sx"};

  if (singleNoRotationGates.find(gateName) != singleNoRotationGates.end() && gateParams.size() != 0) {
    throw std::runtime_error("Error: " + gateName + " gate format is not correct");
  }

  if (it != GateMatrixCache.end()) {
    return it->second;
  }
  auto gateFmtErr = [&]() {
    throw std::runtime_error("Error: " + gateName + " gate format is not correct");
  };
  if (gateName == "rx") {
    if (gateParams.size() != 1)
      gateFmtErr();
    return rx_mat(gateParams);
  }
  if (gateName == "ry") {
    if (gateParams.size() != 1)
      gateFmtErr();
    return ry_mat(gateParams);
  }
  if (gateName == "rz" || gateName == "RZ") {
    if (gateParams.size() != 1)
      gateFmtErr();
    return rz_mat(gateParams);
  }
  if (gateName == "p" || gateName == "u1") {
    if (gateParams.size() != 1)
      gateFmtErr();
    return p_mat(gateParams);
  }
  if (gateName == "u3") {
    if (gateParams.size() != 3)
      gateFmtErr();
    return u3_mat(gateParams);
  }
  if (gateName == "cx") {
    if (gateParams.size() != 0)
      gateFmtErr();
    auto idex_two = qllvm::OP::getbit_from_muti_valueSemanticsInstOp(op);
    if (ForwardOrReverse(idex_two, qbitseq)) {
      return CX_mat_0;
    }
    return CX_mat_1;
  }
  if (gateName == "cy") {
    if (gateParams.size() != 0)
      gateFmtErr();
    auto idex_two = qllvm::OP::getbit_from_muti_valueSemanticsInstOp(op);
    if (ForwardOrReverse(idex_two, qbitseq)) {
      return CY_mat_0;
    }
    return CY_mat_1;
  }
  if (gateName == "cz" || gateName == "CZ") {
    if (gateParams.size() != 0)
      gateFmtErr();
    return CZ_mat;
  }
  if (gateName == "swap") {
    if (gateParams.size() != 0)
      gateFmtErr();
    return SWAP_mat;
  }
  if (gateName == "cp" || gateName == "cphase" || gateName == "crz" || gateName == "cu1") {
    if (gateParams.size() != 1)
      gateFmtErr();
    return CPhase_mat(gateParams);
  }

  throw std::runtime_error("Unknown single qubit gate: " + gateName);
}

Eigen::MatrixXcd getSingleGateMat(const qop_t &in_op) {

  const auto &gateName = in_op.first;
  const auto &gateParams = in_op.second;
  // std::cout<< "op_name: "<< gateName<<std::endl;
  static const std::unordered_set<std::string> singleNoRotationGates = {
      "x", "y", "z", "h", "t", "tdg", "id", "s", "sdg", "sx"};
  if (singleNoRotationGates.count(gateName) && gateParams.size() != 0) {
    throw std::runtime_error("Error: " + gateName + " gate format is not correct");
  }
  static const Eigen::Matrix2cd X_mat = []() {
    Eigen::Matrix2cd result = Eigen::MatrixXcd::Zero(2, 2);
    result << 0.0, 1.0, 1.0, 0.0;
    return result;
  }();
  static const Eigen::Matrix2cd Id_mat = []() {
    Eigen::Matrix2cd result = Eigen::MatrixXcd::Zero(2, 2);
    result << 1.0, 0.0, 0.0, 1.0;
    return result;
  }();
  static const Eigen::Matrix2cd Y_mat = []() {
    Eigen::Matrix2cd result = Eigen::MatrixXcd::Zero(2, 2);
    result << 0.0,-1j, 1j, 0.0;
    return result;
  }();
  static const Eigen::Matrix2cd Z_mat = []() {
    Eigen::Matrix2cd result = Eigen::MatrixXcd::Zero(2, 2);
    result << 1.0, 0.0, 0.0, -1.0;
    return result;
  }();
  static const Eigen::Matrix2cd H_mat = []() {
    Eigen::Matrix2cd result = Eigen::MatrixXcd::Zero(2, 2);
    result << M_SQRT1_2, M_SQRT1_2, M_SQRT1_2, -M_SQRT1_2;
    return result;
  }();
  static const Eigen::Matrix2cd SX_mat = []() {
    Eigen::Matrix2cd result = Eigen::MatrixXcd::Zero(2, 2);
    result << M_SQRT1_2, std::complex<double>(0,-1)*M_SQRT1_2, std::complex<double>(0,-1)*M_SQRT1_2, M_SQRT1_2;
    return result;
  }();

  static const auto rx_mat = [](const std::vector<double> &in_params) {
    assert(in_params.size() == 1);
    auto theta = in_params[0];
    Eigen::Matrix2cd result = Eigen::MatrixXcd::Zero(2, 2);
    result << std::cos(0.5 * theta),
        std::complex<double>(0, -1) * std::sin(0.5 * theta),
        std::complex<double>(0, -1) * std::sin(0.5 * theta),
        std::cos(0.5 * theta);
    return result;
  };

  static const auto ry_mat = [](const std::vector<double> &in_params) {
    assert(in_params.size() == 1);
    auto theta = in_params[0];
    Eigen::Matrix2cd result = Eigen::MatrixXcd::Zero(2, 2);
    result << std::cos(0.5 * theta), -std::sin(0.5 * theta),
        std::sin(0.5 * theta), std::cos(0.5 * theta);
    return result;
  };

  static const auto rz_mat = [](const std::vector<double> &in_params) {
    assert(in_params.size() == 1);
    auto theta = in_params[0];
    Eigen::Matrix2cd result = Eigen::MatrixXcd::Zero(2, 2);
    result << std::exp(std::complex<double>(0, -0.5 * theta)), 0.0, 0.0,
        std::exp(std::complex<double>(0, 0.5 * theta));
    return result;
  };

  static const auto p_mat = [](const std::vector<double> &in_params) {
    assert(in_params.size() == 1);
    auto theta = in_params[0];
    Eigen::Matrix2cd result = Eigen::MatrixXcd::Zero(2, 2);
    result << 1.0, 0.0, 0.0, std::exp(std::complex<double>(0, theta));
    return result;
  };

  static const auto u3_mat = [](const std::vector<double> &in_params) {
    assert(in_params.size() == 3);
    auto in_theta = in_params[0];
    auto in_phi = in_params[1];
    auto in_lambda = in_params[2];

    Eigen::Matrix2cd result = Eigen::MatrixXcd::Zero(2, 2);

    // qpp::cmat gateMat(2, 2);
    result << std::cos(in_theta / 2.0),
        -std::exp(std::complex<double>(0, in_lambda)) *
            std::sin(in_theta / 2.0),
        std::exp(std::complex<double>(0, in_phi)) * std::sin(in_theta / 2.0),
        std::exp(std::complex<double>(0, in_phi + in_lambda)) *
            std::cos(in_theta / 2.0);

    return result;
  };

  static const std::unordered_map<std::string, Eigen::Matrix2cd>
      GateMatrixCache = {{"x", X_mat},           {"y", Y_mat},{"id", Id_mat},
                        {"z", Z_mat},           {"h", H_mat},
                        {"t", p_mat({M_PI_4})}, {"tdg", p_mat({-M_PI_4})},
                        {"s", p_mat({M_PI_2})}, {"sdg", p_mat({-M_PI_2})},
                        {"sx", SX_mat}};
  const auto it = GateMatrixCache.find(gateName);
  if (it != GateMatrixCache.end()) {
    return it->second;
  }
  auto gateFmtErr = [&]() {
    throw std::runtime_error("Error: " + gateName + " gate format is not correct");
  };
  if (gateName == "rx") {
    if (gateParams.size() != 1)
      gateFmtErr();
    return rx_mat(gateParams);
  }
  if (gateName == "ry") {
    if (gateParams.size() != 1)
      gateFmtErr();
    return ry_mat(gateParams);
  }
  if (gateName == "rz" || gateName == "RZ") {
    if (gateParams.size() != 1)
      gateFmtErr();
    return rz_mat(gateParams);
  }
  if (gateName == "p" || gateName == "u1") {
    if (gateParams.size() != 1)
      gateFmtErr();
    return p_mat(gateParams);
  }
  if (gateName == "u3") {
    if (gateParams.size() != 3)
      gateFmtErr();
    return u3_mat(gateParams);
  }

  throw std::runtime_error("Unknown single qubit gate: " + gateName);
}

Eigen::MatrixXcd getGateMat(const qop_t &in_op,int ForwardOrReverse) {
  // std::cout << "=========================getGateMat===========================" << std::endl;
  const auto &gateName = in_op.first;
  const auto &gateParams = in_op.second;
  static const std::unordered_set<std::string> singleNoRotationGates = {
      "x", "y", "z", "h", "t", "tdg", "id", "s", "sdg", "sx"};
  if (singleNoRotationGates.count(gateName) && gateParams.size() != 0) {
    throw std::runtime_error("Error: " + gateName + " gate format is not correct");
  }
  static const Eigen::Matrix2cd X_mat = []() {
    Eigen::Matrix2cd result = Eigen::MatrixXcd::Zero(2, 2);
    result << 0.0, 1.0, 1.0, 0.0;
    return result;
  }();
  static const Eigen::Matrix2cd Id_mat = []() {
    Eigen::Matrix2cd result = Eigen::MatrixXcd::Zero(2, 2);
    result << 1.0, 0.0, 0.0, 1.0;
    return result;
  }();
  static const Eigen::Matrix2cd Y_mat = []() {
    Eigen::Matrix2cd result = Eigen::MatrixXcd::Zero(2, 2);
    result << 0.0,-1j, 1j, 0.0;
    return result;
  }();
  static const Eigen::Matrix2cd Z_mat = []() {
    Eigen::Matrix2cd result = Eigen::MatrixXcd::Zero(2, 2);
    result << 1.0, 0.0, 0.0, -1.0;
    return result;
  }();
  static const Eigen::Matrix2cd H_mat = []() {
    Eigen::Matrix2cd result = Eigen::MatrixXcd::Zero(2, 2);
    result << M_SQRT1_2, M_SQRT1_2, M_SQRT1_2, -M_SQRT1_2;
    return result;
  }();
  static const Eigen::Matrix2cd SX_mat = []() {
    Eigen::Matrix2cd result = Eigen::MatrixXcd::Zero(2, 2);
    result << M_SQRT1_2, std::complex<double>(0,-1)*M_SQRT1_2, std::complex<double>(0,-1)*M_SQRT1_2, M_SQRT1_2;
    return result;
  }();
  static const Eigen::Matrix4cd CX_mat_0 = []() {
    Eigen::Matrix4cd result = Eigen::MatrixXcd::Zero(4,4);
    result << 1.0, 0.0, 0.0, 0.0,
              0.0, 0.0, 0.0, 1.0,
              0.0, 0.0, 1.0, 0.0,
              0.0, 1.0, 0.0, 0.0;
    return result;
  }();
  static const Eigen::Matrix4cd CX_mat_1 = []() {
    Eigen::Matrix4cd result = Eigen::MatrixXcd::Zero(4,4);
    result << 1.0, 0.0, 0.0, 0.0,
              0.0, 1.0, 0.0, 0.0,
              0.0, 0.0, 0.0, 1.0,
              0.0, 0.0, 1.0, 0.0;
    return result;
  }();
  static const Eigen::Matrix4cd CY_mat_0 = []() {
    Eigen::Matrix4cd result = Eigen::MatrixXcd::Zero(4,4);
    result << 1.0, 0.0, 0.0, 0.0,
              0.0, 0.0, 0.0, 1j,
              0.0, 0.0, 1.0, 0.0,
              0.0, -1j, 0.0, 0.0;
    return result;
  }();
  static const Eigen::Matrix4cd CY_mat_1 = []() {
    Eigen::Matrix4cd result = Eigen::MatrixXcd::Zero(4,4);
    result << 1.0, 0.0, 0.0, 0.0,
              0.0, 1.0, 0.0, 0.0,
              0.0, 0.0, 0.0, -1j,
              0.0, 0.0, 1j, 0.0;
    return result;
  }();
  static const Eigen::Matrix4cd CZ_mat = []() {
    Eigen::Matrix4cd result = Eigen::MatrixXcd::Zero(4,4);
    result << 1.0, 0.0, 0.0, 0.0,
              0.0, 1.0, 0.0, 0.0,
              0.0, 0.0, 1.0, 0.0,
              0.0, 0.0, 0.0, -1.0;
    return result;
  }();
  static const Eigen::Matrix4cd SWAP_mat = []() {
    Eigen::Matrix4cd result = Eigen::MatrixXcd::Zero(4,4);
    result << 1.0, 0.0, 0.0, 0.0,
              0.0, 0.0, 1.0, 0.0,
              0.0, 1.0, 0.0, 0.0,
              0.0, 0.0, 0.0, 1.0;
    return result;
  }();

  static const auto CPhase_mat = [](const std::vector<double> &in_params) {
    auto theta = in_params[0];
    Eigen::Matrix4cd result = Eigen::MatrixXcd::Zero(4, 4);
    result << 1.0, 0.0, 0.0, 0.0,
              0.0, 1.0, 0.0, 0.0,
              0.0, 0.0, 1.0, 0.0,
              0.0, 0.0, 0.0, std::exp(std::complex<double>(0,theta));
    return result;
  };

  static const auto rx_mat = [](const std::vector<double> &in_params) {
    assert(in_params.size() == 1);
    auto theta = in_params[0];
    Eigen::Matrix2cd result = Eigen::MatrixXcd::Zero(2, 2);
    result << std::cos(0.5 * theta),
        std::complex<double>(0, -1) * std::sin(0.5 * theta),
        std::complex<double>(0, -1) * std::sin(0.5 * theta),
        std::cos(0.5 * theta);
    return result;
  };

  static const auto ry_mat = [](const std::vector<double> &in_params) {
    assert(in_params.size() == 1);
    auto theta = in_params[0];
    Eigen::Matrix2cd result = Eigen::MatrixXcd::Zero(2, 2);
    result << std::cos(0.5 * theta), -std::sin(0.5 * theta),
        std::sin(0.5 * theta), std::cos(0.5 * theta);
    return result;
  };

  static const auto rz_mat = [](const std::vector<double> &in_params) {
    assert(in_params.size() == 1);
    auto theta = in_params[0];
    Eigen::Matrix2cd result = Eigen::MatrixXcd::Zero(2, 2);
    result << std::exp(std::complex<double>(0, -0.5 * theta)), 0.0, 0.0,
        std::exp(std::complex<double>(0, 0.5 * theta));
    return result;
  };

  static const auto p_mat = [](const std::vector<double> &in_params) {
    assert(in_params.size() == 1);
    auto theta = in_params[0];
    Eigen::Matrix2cd result = Eigen::MatrixXcd::Zero(2, 2);
    result << 1.0, 0.0, 0.0, std::exp(std::complex<double>(0, theta));
    return result;
  };

  static const auto u3_mat = [](const std::vector<double> &in_params) {
    assert(in_params.size() == 3);
    auto in_theta = in_params[0];
    auto in_phi = in_params[1];
    auto in_lambda = in_params[2];

    Eigen::Matrix2cd result = Eigen::MatrixXcd::Zero(2, 2);
    // qpp::cmat gateMat(2, 2);
    result << std::cos(in_theta / 2.0),
        -std::exp(std::complex<double>(0, in_lambda)) *
            std::sin(in_theta / 2.0),
        std::exp(std::complex<double>(0, in_phi)) * std::sin(in_theta / 2.0),
        std::exp(std::complex<double>(0, in_phi + in_lambda)) *
            std::cos(in_theta / 2.0);

    return result;
  };

  static const std::unordered_map<std::string, Eigen::Matrix2cd>
      GateMatrixCache = {{"x", X_mat},           {"y", Y_mat},
                        {"z", Z_mat},           {"h", H_mat},
                        {"t", p_mat({M_PI_4})}, {"tdg", p_mat({-M_PI_4})},
                        {"s", p_mat({M_PI_2})}, {"sdg", p_mat({-M_PI_2})},
                        {"sx", SX_mat}, {"id", Id_mat}};

  const auto it = GateMatrixCache.find(gateName);
  if (it != GateMatrixCache.end()) {
    return it->second;
  }
  auto gateFmtErr = [&]() {
    throw std::runtime_error("Error: " + gateName + " gate format is not correct");
  };
  if (gateName == "rx") {
    if (gateParams.size() != 1)
      gateFmtErr();
    return rx_mat(gateParams);
  }
  if (gateName == "ry") {
    if (gateParams.size() != 1)
      gateFmtErr();
    return ry_mat(gateParams);
  }
  if (gateName == "rz" || gateName == "RZ") {
    if (gateParams.size() != 1)
      gateFmtErr();
    return rz_mat(gateParams);
  }
  if (gateName == "p" || gateName == "u1") {
    if (gateParams.size() != 1)
      gateFmtErr();
    return p_mat(gateParams);
  }
  if (gateName == "u3") {
    if (gateParams.size() != 3)
      gateFmtErr();
    return u3_mat(gateParams);
  }
  if (gateName == "cx") {
    if (gateParams.size() != 0)
      gateFmtErr();
    if (ForwardOrReverse == 0) {
      return CX_mat_0;
    }
    return CX_mat_1;
  }
  if (gateName == "cy") {
    if (gateParams.size() != 0)
      gateFmtErr();
    if (ForwardOrReverse == 0) {
      return CY_mat_0;
    }
    return CY_mat_1;
  }
  if (gateName == "cz" || gateName == "CZ") {
    if (gateParams.size() != 0)
      gateFmtErr();
    return CZ_mat;
  }
  if (gateName == "swap") {
    if (gateParams.size() != 0)
      gateFmtErr();
    return SWAP_mat;
  }
  if (gateName == "cp" || gateName == "cphase" || gateName == "crz" || gateName == "cu1") {
    if (gateParams.size() != 1)
      gateFmtErr();
    return CPhase_mat(gateParams);
  }

  throw std::runtime_error("Unknown single qubit gate: " + gateName);
}

Eigen::MatrixXcd getGateMat(mlir::quantum::ValueSemanticsInstOp &op,int i){
  auto op_name = op.name().str();
  const auto parameters = get_parameters(op);
  std::unordered_set<std::string> singleNoRotationGates = {
      "id", "x", "y", "z", "h", "t", "tdg", "s", "sdg", "sx"};
  

  if (singleNoRotationGates.find(op_name) != singleNoRotationGates.end() && parameters.size() != 0) {
    throw std::runtime_error("Error: " + op_name + " gate format is not correct");
  }

  if ((op_name == "id" || op_name == "Id") && parameters.size() == 0 && op.getOperands().size() == 1) {
    Eigen::MatrixXcd result = Eigen::MatrixXcd::Zero(2, 2);
    result << 1.0, 0.0, 
              0.0, 1.0;
    return result;
  }else if((op_name == "x" || op_name == "X") && parameters.size() == 0 && op.getOperands().size() == 1){
    Eigen::MatrixXcd result = Eigen::MatrixXcd::Zero(2, 2);
    result << 0.0, 1.0, 
              1.0, 0.0;
    return result;
  }else if((op_name == "y" || op_name == "Y") && parameters.size() == 0 && op.getOperands().size() == 1){  
      Eigen::MatrixXcd result = Eigen::MatrixXcd::Zero(2, 2);
      result << 0.0, -1j,
                 1j, 0.0;
      return result;
    }else if((op_name == "z" || op_name == "Z") && parameters.size() == 0 && op.getOperands().size() == 1){
      Eigen::MatrixXcd result = Eigen::MatrixXcd::Zero(2, 2);
      result << 1.0, 0.0, 
                0.0, -1.0;
      return result;
    }else if((op_name == "rx" || op_name == "Rx") && parameters.size() == 1 && op.getOperands().size() == 2){
      mlir::Value param = op.getOperand(1);
      auto theta = qllvm::OP::tryGetConstAngle(param);
      Eigen::MatrixXcd result = Eigen::MatrixXcd::Zero(2, 2);
      result << std::cos(0.5 * theta),std::complex<double>(0,-1) * std::sin(0.5 * theta),
        std::complex<double>(0,-1) * std::sin(0.5 * theta),std::cos(0.5 * theta);
      return result;
    }else if((op_name == "ry" || op_name == "Ry") && parameters.size() == 1 && op.getOperands().size() == 2){
      mlir::Value param = op.getOperand(1);
      auto theta = qllvm::OP::tryGetConstAngle(param);
      Eigen::MatrixXcd result = Eigen::MatrixXcd::Zero(2, 2);
      result << std::cos(0.5 * theta), -std::sin(0.5 * theta),
        std::sin(0.5 * theta), std::cos(0.5 * theta);
      return result;
    }else if((op_name == "rz" || op_name == "Rz" || op_name == "RZ") && parameters.size() == 1 && op.getOperands().size() == 2){
      mlir::Value param = op.getOperand(1);
      auto theta = qllvm::OP::tryGetConstAngle(param);
      Eigen::MatrixXcd result = Eigen::MatrixXcd::Zero(2, 2);
      result << std::exp(std::complex<double>(0, -0.5 * theta)), 0.0,
                0.0,std::exp(std::complex<double>(0, 0.5 * theta));
      return result;
    }else if((op_name == "h" || op_name == "H") && parameters.size() == 0 && op.getOperands().size() == 1){
      Eigen::MatrixXcd result = Eigen::MatrixXcd::Zero(2, 2);
      result << M_SQRT1_2, M_SQRT1_2, M_SQRT1_2, -M_SQRT1_2;
      return result;
    }else if((op_name == "t") && parameters.size() == 0 && op.getOperands().size() == 1){
      Eigen::MatrixXcd result = Eigen::MatrixXcd::Zero(2, 2);
      result << 1.0, 0.0, 
                0.0, std::exp(std::complex<double>(0, M_PI_4));
      return result;
    }else if((op_name == "tdg") && parameters.size() == 0 && op.getOperands().size() == 1){
      Eigen::MatrixXcd result = Eigen::MatrixXcd::Zero(2, 2);
      result << 1.0, 0.0, 
                0.0, std::exp(std::complex<double>(0, -1*M_PI_4));
      return result;
    }else if((op_name == "s") && parameters.size() == 0 && op.getOperands().size() == 1){
      Eigen::MatrixXcd result = Eigen::MatrixXcd::Zero(2, 2);
      result << 1.0, 0.0,
                0.0, std::exp(std::complex<double>(0, M_PI_2));
      return result;
    }else if((op_name == "sdg") && parameters.size() == 0 && op.getOperands().size() == 1){
      Eigen::MatrixXcd result = Eigen::MatrixXcd::Zero(2, 2);
      result << 1.0, 0.0,
                 0.0, std::exp(std::complex<double>(0, -1*M_PI_2));
      return result;
    }else if((op_name == "cx" || op_name == "CX") && i == 0 && parameters.size() == 0 && op.getOperands().size() == 2){
      Eigen::MatrixXcd result = Eigen::MatrixXcd::Zero(4, 4);
      result << 1.0, 0.0, 0.0, 0.0,
                0.0, 0.0, 0.0, 1.0,
                0.0, 0.0, 1.0, 0.0,
                0.0, 1.0, 0.0, 0.0;
      return result;
    }else if((op_name == "cx" || op_name == "CX") && i == 1 && parameters.size() == 0 && op.getOperands().size() == 2){
      Eigen::MatrixXcd result = Eigen::MatrixXcd::Zero(4, 4);
      result << 1.0, 0.0, 0.0, 0.0,
                0.0, 1.0, 0.0, 0.0,
                0.0, 0.0, 0.0, 1.0,
                0.0, 0.0, 1.0, 0.0;
      return result;
    }else if((op_name == "cy" || op_name == "CY") && i == 0 && parameters.size() == 0 && op.getOperands().size() == 2){
      Eigen::MatrixXcd result = Eigen::MatrixXcd::Zero(4, 4);
      result << 1.0, 0.0, 0.0, 0.0,
                0.0, 0.0, 0.0, 1j,
                0.0, 0.0, 1.0, 0.0,
                0.0, -1j, 0.0, 0.0;

      return result;
    }else if((op_name == "cy" || op_name == "CY") && i == 1 && parameters.size() == 0 && op.getOperands().size() == 2){
      Eigen::MatrixXcd result = Eigen::MatrixXcd::Zero(4, 4);
      result << 1.0, 0.0, 0.0, 0.0,
                0.0, 1.0, 0.0, 0.0,
                0.0, 0.0, 0.0, -1j,
                0.0, 0.0, 1j, 0.0;
      return result;
    }else if((op_name == "cz" || op_name == "CZ") && parameters.size() == 0 && op.getOperands().size() == 2){
      Eigen::MatrixXcd result = Eigen::MatrixXcd::Zero(4, 4);
      result << 1.0, 0.0, 0.0, 0.0,
                0.0, 1.0, 0.0, 0.0,
                0.0, 0.0, 1.0, 0.0,
                0.0, 0.0, 0.0, -1.0;
      return result;
    }else if(op_name == "u3" && parameters.size() == 3 && op.getOperands().size() == 4){
      mlir::Value theta_v = op.getOperand(1);
      auto theta = qllvm::OP::tryGetConstAngle(theta_v);
      mlir::Value phi_v = op.getOperand(2);
      auto phi = qllvm::OP::tryGetConstAngle(phi_v);
      mlir::Value lambda_v = op.getOperand(3);
      auto lambda = qllvm::OP::tryGetConstAngle(lambda_v);
      Eigen::MatrixXcd result = Eigen::MatrixXcd::Zero(2, 2);
      result << std::cos(0.5 * theta),
        -std::sin(0.5 * theta)*std::exp(std::complex<double>(0, lambda)),
        std::sin(0.5 * theta)*std::exp(std::complex<double>(0, phi)),
        std::cos(0.5 * theta)*std::exp(std::complex<double>(0, (phi+lambda)));
      // std::cout << "u3 result " << result << std::endl;
        return result;
    }else if(op_name == "cp" && parameters.size() == 1 && op.getOperands().size() == 3){
      mlir::Value theta_v = op.getOperand(2);
      auto theta = qllvm::OP::tryGetConstAngle(theta_v);
      Eigen::MatrixXcd result = Eigen::MatrixXcd::Zero(4, 4);
      result << 1,0,0,0,
                0,1,0,0,
                0,0,1,0,
                0,0,0,std::exp(std::complex<double>(0,-1*theta));
      return result;
    }else if(op_name == "swap" && parameters.size() == 0 && op.getOperands().size() == 2 ){
      Eigen::MatrixXcd result = Eigen::MatrixXcd::Zero(4, 4);
      result << 1.0, 0.0, 0.0, 0.0,
                0.0, 0.0, 1.0, 0.0,
                0.0, 1.0, 0.0, 0.0,
                0.0, 0.0, 0.0, 1.0;
      return result;
    }else if(op_name == "ryy" && parameters.size() == 1 && op.getOperands().size() == 3){
      mlir::Value lambda_v = op.getOperand(2);
      auto lambda = qllvm::OP::tryGetConstAngle(lambda_v);
      Eigen::MatrixXcd result = Eigen::MatrixXcd::Zero(4, 4);
      result << std::cos(0.5*lambda),0,0,std::complex<double>(0,1)*std::sin(0.5*lambda),
                0,std::cos(0.5*lambda),std::complex<double>(0,-1)*std::sin(0.5*lambda),0,
                0,std::complex<double>(0,-1)*std::sin(0.5*lambda),std::cos(0.5*lambda),0,
                std::complex<double>(0,1)*std::sin(0.5*lambda),0,0,std::cos(0.5*lambda);
      return result;
    }else if(op_name == "p" && parameters.size() == 1 && op.getOperands().size() == 2){
      mlir::Value param = op.getOperand(1);
      auto theta = qllvm::OP::tryGetConstAngle(param);
      Eigen::MatrixXcd result = Eigen::MatrixXcd::Zero(2, 2);
      result << std::exp(std::complex<double>(0, -0.5 * theta)), 0.0,
                0.0,std::exp(std::complex<double>(0, 0.5 * theta));
      return result;
    }else if(op_name == "sx" && parameters.size() == 0 && op.getOperands().size() == 1){
      Eigen::MatrixXcd result = Eigen::MatrixXcd::Zero(2, 2);
      result << M_SQRT1_2, std::complex<double>(0,-1)*M_SQRT1_2, std::complex<double>(0,-1)*M_SQRT1_2, M_SQRT1_2;
      return result;
    }
  throw std::runtime_error("Unknown qubit gate: " + op_name);

  return Eigen::MatrixXcd::Zero(2,2);
}


}
} // namespace

