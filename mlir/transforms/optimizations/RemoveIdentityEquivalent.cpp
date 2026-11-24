/*******************************************************************************
 * Copyright (c) 2018-, UT-Battelle, LLC.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the MIT License
 * which accompanies this distribution.
 *
 * Contributors:
 *   Alexander J. McCaskey - initial API and implementation
 *   Thien Nguyen - implementation
 *
 * Modified by QCFlow (2026) for QLLVM project.
 *******************************************************************************/
#include "utils/mutation_tracking.hpp"
#include "RemoveIdentityEquivalent.hpp"
#include "Quantum/QuantumOps.h"
#include "mlir/Dialect/LLVMIR/LLVMDialect.h"
#include "mlir/Dialect/StandardOps/IR/Ops.h"
#include "mlir/IR/Matchers.h"
#include "mlir/IR/PatternMatch.h"
#include "mlir/Pass/Pass.h"
#include "mlir/Pass/PassManager.h"
#include "mlir/Target/LLVMIR.h"
#include "mlir/Transforms/DialectConversion.h"
#include "mlir/Transforms/Passes.h"
#include <iostream>
#include <algorithm>
#include <unordered_map>
#include <tr1/unordered_map>
#include "utils/circuit.hpp"
#include "utils/get_matrix.hpp"
#include "utils/op.hpp"
#include "utils/approximation_budget.hpp"
#include <Eigen/Dense>
#include <eigen3/unsupported/Eigen/KroneckerProduct>
#include <eigen3/unsupported/Eigen/MatrixFunctions>
namespace qllvm {
void RemoveIdentityEquivalent::getDependentDialects(
    DialectRegistry &registry) const {
  registry.insert<LLVM::LLVMDialect>();
}


void RemoveIdentityEquivalent::runOnOperation() {
  // std::cout << "RemoveIdentityEquivalent: " << std::endl;
  if(syn&&*e == 0)
    return;
  if (f == true)
    *c+=1;
  if(printCountAndDepth||syn){
    circuit::getGateCountAndTopOp(before_gate_count, top_op, getOperation());
    int depth = 0;
    if (*c_d==0){
      for (auto &op : top_op) {
        depth = circuit::getDepth(op);
        before_circuit_depth = depth > before_circuit_depth ? depth : before_circuit_depth;
      }
    }else{
      before_circuit_depth = *c_d;
    }
  }
  std::unordered_map<std::string, int> qbit_vect;
  int counts =0;
  getOperation().walk([&](mlir::quantum::QallocOp op) {
    qbit_vect.emplace(op.name().str(),counts++);
  });

  std::vector<mlir::quantum::ValueSemanticsInstOp> deadOps;
  struct Candidate {
    mlir::quantum::ValueSemanticsInstOp op;
    double angle;
    double distance;
  };
  std::vector<Candidate> candidates;
  // Walk the operations within the function.
  getOperation().walk([&](mlir::quantum::ValueSemanticsInstOp op) {

    // Modified 2026-11-19: this walk visits each operation once and only marks
    // the current operation for deletion. No forward scan inserts other ops,
    // so scanning the entire deletion list here was redundant and quadratic.
    // A sub-block global phase can become relative under coherent control.
    if (op->getParentOfType<mlir::quantum::CtrlURegion>()) return;
    auto inst_name = op.name().str();
    if (inst_name == "rz" || inst_name == "p" || inst_name == "u1"|| inst_name == "cp" || inst_name == "crz"
        || inst_name == "cphase"|| inst_name == "rx"|| inst_name == "ry"|| inst_name == "u1"){
      if (op.getNumResults() == 1 && op.getNumOperands() == 2) {
        const double angle = OP::tryGetConstAngle(op.getOperand(1));
        // Modified 2026-11-22: 1-|cos(theta/2)|, evaluated without subtracting
        // nearly equal floats. Controlled/multi-qubit gates stay excluded.
        const double sine = std::sin(std::remainder(angle, 2 * M_PI) / 4);
        if (2 * sine * sine < MINIMUM_TOL)
          candidates.push_back({op, angle, approximation::rotationDistanceBound(angle)});
      }
    }
  });

  // A few larger rotations encountered first must not consume the budget
  // before thousands of almost-zero QFT angles. Use the same conservative
  // charge as before, with stable program order for equal costs.
  std::stable_sort(candidates.begin(), candidates.end(),
                   [](const Candidate &a, const Candidate &b) {
                     return a.distance < b.distance;
                   });
  for (auto &candidate : candidates) {
    if (!approximation::chargeRotation(candidate.angle)) continue;
    auto op = candidate.op;
    op.getResult(0).replaceAllUsesWith(op.getOperand(0));
    deadOps.push_back(op);
  }

  for (auto &op : deadOps) {
      op->dropAllUses();
      optimization::markChanged();
      op.erase();
  }
} // namespace qllvm
}
