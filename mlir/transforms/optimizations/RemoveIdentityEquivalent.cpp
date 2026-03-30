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
#include <unordered_map>
#include <tr1/unordered_map>
#include "utils/circuit.hpp"
#include "utils/get_matrix.hpp"
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
  // Walk the operations within the function.
  getOperation().walk([&](mlir::quantum::ValueSemanticsInstOp op) {
    
    if (std::find(deadOps.begin(), deadOps.end(), op) != deadOps.end()) {
        return;
    }
    auto inst_name = op.name().str();
    if (inst_name == "rz" || inst_name == "p" || inst_name == "u1"|| inst_name == "cp" || inst_name == "crz" 
        || inst_name == "cphase"|| inst_name == "rx"|| inst_name == "ry"|| inst_name == "u1"){
      Eigen::MatrixXcd U = qllvm::matrix::getGateMat(op, qbit_vect);
      // calculate the trace of the matrix
      std::complex<double> trace = U.trace();
      // get the dimension of the matrix
      int d = U.rows();
      // calculate the normalized magnitude of the trace
      double normalized_magnitude = std::abs(trace) / d;

      if(op.getNumResults() == 1 && 1-normalized_magnitude < MINIMUM_TOL){
          (op.getResult(0))
                  .replaceAllUsesWith(op.getOperand(0));
          deadOps.emplace_back(op);
      }else if(op.getNumResults() == 1 && 1-normalized_magnitude < 3*MINIMUM_TOL/5) {
          op.getResult(0).replaceAllUsesWith(op.getOperand(0));
          op.getResult(1).replaceAllUsesWith(op.getOperand(1));
          deadOps.emplace_back(op);
      }
    }
  });
  
  for (auto &op : deadOps) {
      op->dropAllUses();
      op.erase();
  }
} // namespace qllvm
}

          
          