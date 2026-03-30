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
#include "trans_u3_ro_rphi.hpp"
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
#include "utils/gate_matrix.hpp"
#include <iostream>
#include <iomanip> 
#include "utils/circuit.hpp"
#include "utils/op.hpp"
#include "gen_qasm.hpp"

namespace qllvm {
void trans_u3_ro_rphi::getDependentDialects(
    DialectRegistry &registry) const {
  registry.insert<LLVM::LLVMDialect>();
}

void trans_u3_rphi(mlir::quantum::ValueSemanticsInstOp &op,std::vector<mlir::quantum::ValueSemanticsInstOp> &deadOps){
  deadOps.emplace_back(op);

  std::vector<mlir::Value> params_values;
  mlir::OpBuilder rewriter(op);
  rewriter.setInsertionPointAfter(op);

  mlir::Value inputQubit = op.getOperand(0);
  mlir::Type qubit_type = inputQubit.getType();
  auto theta_1 = op.getOperand(1);
  auto theta_2 = op.getOperand(2);
  auto theta_3 = op.getOperand(3);

  auto theta_val_1 = qllvm::OP::tryGetConstAngle(theta_1);
  auto theta_val_2 = qllvm::OP::tryGetConstAngle(theta_2);
  auto theta_val_3 = qllvm::OP::tryGetConstAngle(theta_3);

  mlir::Value rz_theta = rewriter.create<mlir::ConstantOp>(
                                op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), theta_val_2 + theta_val_3-theta_val_1));
  mlir::Value rphi_theta_2 = rewriter.create<mlir::ConstantOp>(
                                op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), M_PI_2 + theta_val_3 - theta_val_2));
  mlir::Value rphi_theta_3 = rewriter.create<mlir::ConstantOp>(
                                op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), theta_val_3 - M_PI_2));
  mlir::Value rphi_M_PI_2 = rewriter.create<mlir::ConstantOp>(
                                  op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(),  M_PI_2));

  auto new_inst = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "rz", llvm::makeArrayRef({inputQubit}),
                      llvm::makeArrayRef({rz_theta}));
  inputQubit = new_inst.getResult(0);

  auto new_inst_1 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "rphi", llvm::makeArrayRef({inputQubit}),
                      llvm::makeArrayRef({rphi_M_PI_2,rphi_theta_2}));
  inputQubit = new_inst_1.getResult(0);

  auto new_inst_2 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                    op.getLoc(), llvm::makeArrayRef({qubit_type}),
                    "rphi", llvm::makeArrayRef({inputQubit}),
                    llvm::makeArrayRef({rphi_M_PI_2,rphi_theta_3}));
  inputQubit = new_inst_2.getResult(0);

  op.getResult(0).replaceAllUsesWith(inputQubit);
}

void trans_u3_ro_rphi::runOnOperation() {

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

  // Walk the operations within the function.
  std::vector<mlir::quantum::ValueSemanticsInstOp> deadOps;
  getOperation().walk([&](mlir::quantum::ValueSemanticsInstOp op) {
    if(op.name() == "u3"){
      trans_u3_rphi(op,deadOps);
    }
  });

  for (auto &op : deadOps) {
    op->dropAllUses();
    op.erase();
  }
  // qllvm::gen_qasm(getOperation(),"after.qasm");
  
  if(printCountAndDepth||syn){
    circuit::getGateCountAndTopOp(after_gate_count, top_op, getOperation());
    int depth = 0;
    for (auto &op : top_op) {
      depth = circuit::getDepth(op);
      after_circuit_depth = depth > after_circuit_depth ? depth : after_circuit_depth;
      *c_d = after_circuit_depth;
    }
    if(printCountAndDepth){
      *p += (before_gate_count-after_gate_count);
      *q += (before_circuit_depth-after_circuit_depth);
    }
    if(syn){
      if((before_gate_count-after_gate_count) == 0 && (before_circuit_depth-after_circuit_depth) == 0){
        *o += 1;
        if(*o == 5)
          *e = 0;
        else
          *e = 1;
      }else{
        *o = 0;
      }
    }
  }
}
} // namespace qllvm

