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
#include "turn_to_phasedx.hpp"
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
#include "utils/get_matrix.hpp"
#include <iostream>
#include <iomanip> 
#include "utils/circuit.hpp"
#include "utils/op.hpp"
#include "ConsolidateBlocks.hpp"

namespace qllvm {
void turn_to_phasedx::getDependentDialects(
    DialectRegistry &registry) const {
  registry.insert<LLVM::LLVMDialect>();
}

void trans_TK1_PhasedX(mlir::quantum::ValueSemanticsInstOp &op,std::vector<mlir::quantum::ValueSemanticsInstOp> &deadOps){
  deadOps.emplace_back(op);
  mlir::Value inputQubit = op.getOperand(0);
  mlir::Type qubit_type = inputQubit.getType();

  mlir::Value alpha_v = op.getOperand(1);
  mlir::Value beta_v = op.getOperand(2);
  mlir::Value gamma_v = op.getOperand(3);

  auto alpha = qllvm::OP::tryGetConstAngle(alpha_v);
  auto beta = qllvm::OP::tryGetConstAngle(beta_v);
  auto gamma = qllvm::OP::tryGetConstAngle(gamma_v);

  std::vector<mlir::Value> params_values;
  mlir::OpBuilder rewriter(op);
  rewriter.setInsertionPointAfter(op);

  mlir::Value neg_one_v = rewriter.create<mlir::ConstantOp>(
                          op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), -1));

  mlir::Value theta1_v = rewriter.create<mlir::ConstantOp>(
                          op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), (alpha-gamma)/2.0));
  mlir::Value theta2_v = rewriter.create<mlir::ConstantOp>(
                          op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), (1+beta)));

  
  auto new_inst = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "phasedx", llvm::makeArrayRef({inputQubit}),
                      llvm::makeArrayRef({neg_one_v,theta1_v}));
  inputQubit = new_inst.getResult(0);
  auto new_inst2 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "phasedx", llvm::makeArrayRef({inputQubit}),
                      llvm::makeArrayRef({theta2_v,alpha_v}));
  inputQubit = new_inst2.getResult(0);
  
  op.getResult(0).replaceAllUsesWith(inputQubit);
}

void turn_run_to_tk1(std::vector<mlir::quantum::ValueSemanticsInstOp> &run,std::vector<mlir::quantum::ValueSemanticsInstOp> &deadOps){
  for(auto &op : run){
    deadOps.emplace_back(op);
  }
  std::vector<mlir::Value> params_values;
  std::vector<mlir::quantum::ValueSemanticsInstOp> block;
  block = run;
  reverse(block.begin(),block.end());
  Eigen::MatrixXcd total_mat = Eigen::MatrixXcd::Identity(2, 2);
  for(auto &op: block){
   auto current_mat =  qllvm::matrix::getGateMat(op,0);
   total_mat = total_mat * current_mat;
  }

  std::vector<std::string> basis_names = {"ZXZ"};

  std::vector<qllvm::utils::EulerBasis> target_basis_set;
  

  for(int i = 0;i < basis_names.size();i++){
      auto basis = qllvm::utils::euler_Basis_FromStr(basis_names[i]);
      target_basis_set.emplace_back(basis);
  }

  auto simplified_seq = qllvm::utils::leastcost_basis(total_mat,target_basis_set);
  if(simplified_seq.size() == 0){
    auto first = run.front();
    auto last = run.back();
    mlir::Value inputQubit = first->getOperand(0);
    last->getResult(0).replaceAllUsesWith(inputQubit);
    for (auto &op_to_delete : run) {
      deadOps.emplace_back(op_to_delete);
    }
    return;
  }

  std::vector<double> params;
  if(simplified_seq.size() ==3){
    for(auto &[pauli_inst, thetas]: simplified_seq){
      params.emplace_back(thetas[0]);
    }
  }else if(simplified_seq.size() == 2){
    if(simplified_seq[0].first == "rz" && simplified_seq[1].first == "rx"){
      params.emplace_back(simplified_seq[0].second[0]);
      params.emplace_back(simplified_seq[1].second[0]);
      params.emplace_back(0.0);
    }else if(simplified_seq[0].first == "rx" && simplified_seq[1].first == "rz"){
      params.emplace_back(0.0);
      params.emplace_back(simplified_seq[0].second[0]);
      params.emplace_back(simplified_seq[1].second[0]);
    }else if(simplified_seq[0].first == "rz" && simplified_seq[1].first == "rz"){
      params.emplace_back(simplified_seq[0].second[0]);
      params.emplace_back(0.0);
      params.emplace_back(simplified_seq[1].second[0]);
    }
  }else if(simplified_seq.size() == 1){
    if(simplified_seq[0].first == "rz"){
      params.emplace_back(simplified_seq[0].second[0]);
      params.emplace_back(0.0);
      params.emplace_back(0.0);
    }else if(simplified_seq[0].first == "rx"){
      params.emplace_back(0.0);
      params.emplace_back(simplified_seq[0].second[0]);
      params.emplace_back(0.0);
    }
  }

  auto last = run.back();
  auto first = run.front();

  mlir::OpBuilder rewriter(last);
  rewriter.setInsertionPointAfter(last);
  
  for(auto theta: params){
    auto theta_val = rewriter.create<mlir::ConstantOp>(
                          last.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), theta/M_PI));
    params_values.emplace_back(theta_val);
  }
  
  mlir::Value inputQubit = first.getOperand(0);
  mlir::Type qubit_type = inputQubit.getType();

  auto new_inst = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      last.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "TK1", llvm::makeArrayRef({inputQubit}),
                      llvm::makeArrayRef(params_values));
  last.getResult(0).replaceAllUsesWith(new_inst.getResult(0));
  trans_TK1_PhasedX(new_inst,deadOps);
}

void turn_to_phasedx::runOnOperation() {
  getOperation().walk([&](mlir::quantum::ValueSemanticsInstOp op) {
    mlir::OpBuilder rewriter(op);
    op.getOperation()->setAttr(llvm::StringRef("onebit"),mlir::IntegerAttr::get(rewriter.getI32Type(), 0));
  });

  // Walk the operations within the function.
  std::vector<mlir::quantum::ValueSemanticsInstOp> deadOps;
  std::vector<std::vector<mlir::quantum::ValueSemanticsInstOp>> runs;
  getOperation().walk([&](mlir::quantum::ValueSemanticsInstOp op) {
    int onebit = op.getOperation()->getAttrOfType<mlir::IntegerAttr>(llvm::StringRef("onebit")).getInt();
    std::vector<mlir::quantum::ValueSemanticsInstOp> current_run;
    if(onebit == 0 && op.getNumResults() == 1){
      current_run.emplace_back(op);
      find_runs(current_run,op,1);
      runs.emplace_back(current_run);
    }
  });

  deadOps.clear();
  for(auto &run : runs){
    turn_run_to_tk1(run,deadOps);
  }

  for (auto &op : deadOps) {
    op->dropAllUses();
    op.erase();
  }

}
} // namespace qllvm

