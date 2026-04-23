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
#include "DecomposemultiPass.hpp"
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
#include "utils/circuit.hpp"
#include "utils/op.hpp"
#include "gen_qasm.hpp"
#include <unordered_set>
namespace qllvm {
void DecomposemultiPass::getDependentDialects(DialectRegistry &registry) const {
  registry.insert<LLVM::LLVMDialect>();
}

 // {"u1", R"#(gate u1(lambda) q { u3(0,0,lambda) q; })#"},
void decompose_u1_gate(mlir::quantum::ValueSemanticsInstOp &op,std::vector<mlir::quantum::ValueSemanticsInstOp> &deadOps){
  deadOps.emplace_back(op);

  mlir::OpBuilder rewriter(op);
  rewriter.setInsertionPointAfter(op);

  std::vector<mlir::Value> param;
  mlir::Value inputQubit = op.getOperand(0);
  mlir::Value theta = op.getOperand(1);
  mlir::Value theta_zero = rewriter.create<mlir::ConstantOp>(
                    op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), 0));

  param.emplace_back(theta_zero);
  param.emplace_back(theta_zero);
  param.emplace_back(theta);
  
  mlir::Type qubit_type = inputQubit.getType();

  auto new_inst = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "u3", llvm::makeArrayRef({inputQubit}),
                      llvm::makeArrayRef(param));
  inputQubit = new_inst.getResult(0);
  op.getResult(0).replaceAllUsesWith(inputQubit);
}

 // {"u2", R"#(gate u2(phi,lambda) q { u3(pi/2,phi,lambda) q; })#"},
void decompose_u2_gate(mlir::quantum::ValueSemanticsInstOp &op,std::vector<mlir::quantum::ValueSemanticsInstOp> &deadOps){
  deadOps.emplace_back(op);
  mlir::OpBuilder rewriter(op);
  rewriter.setInsertionPointAfter(op);

  std::vector<mlir::Value> param;
  mlir::Value inputQubit = op.getOperand(0);
  mlir::Value theta_1 = op.getOperand(1);
  mlir::Value theta_2 = op.getOperand(2);
  mlir::Value theta_pi_2 = rewriter.create<mlir::ConstantOp>(
                    op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), M_PI_2));

  param.emplace_back(theta_pi_2);
  param.emplace_back(theta_1);
  param.emplace_back(theta_2);
  
  mlir::Type qubit_type = inputQubit.getType();

  auto new_inst = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "u3", llvm::makeArrayRef({inputQubit}),
                      llvm::makeArrayRef(param));
  op.getResult(0).replaceAllUsesWith(new_inst.getResult(0));

}

 // {"u", R"#(gate u(theta,phi,lambda) q { u3(theta,phi,lambda) q; })#"},
 // {"u0", R"#(gate u0(theta,phi,lambda) q { u3(theta,phi,lambda) q; })#"},
void decompose_u_gate(mlir::quantum::ValueSemanticsInstOp &op,std::vector<mlir::quantum::ValueSemanticsInstOp> &deadOps){
  deadOps.emplace_back(op);
  mlir::OpBuilder rewriter(op);
  rewriter.setInsertionPointAfter(op);

  std::vector<mlir::Value> param;
  mlir::Value inputQubit = op.getOperand(0);
  mlir::Value theta_1 = op.getOperand(1);
  mlir::Value theta_2 = op.getOperand(2);
  mlir::Value theta_3 = op.getOperand(3);


  param.emplace_back(theta_1);
  param.emplace_back(theta_2);
  param.emplace_back(theta_3);
  
  mlir::Type qubit_type = inputQubit.getType();

  auto new_inst = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "u3", llvm::makeArrayRef({inputQubit}),
                      llvm::makeArrayRef(param));
  op.getResult(0).replaceAllUsesWith(*new_inst.result_begin());
}

 // {"sxdg", R"#(gate sxdg a { s a; h a; s a; })#"},
void decompose_sxdg_gate(mlir::quantum::ValueSemanticsInstOp &op,std::vector<mlir::quantum::ValueSemanticsInstOp> &deadOps){
  deadOps.emplace_back(op);
  mlir::OpBuilder rewriter(op);
  rewriter.setInsertionPointAfter(op);

  std::vector<mlir::Value> param;
  mlir::Value inputQubit = op.getOperand(0);

  mlir::Type qubit_type = inputQubit.getType();

  auto new_inst_1 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "s", llvm::makeArrayRef({inputQubit}),
                      llvm::None);
  inputQubit =  new_inst_1.getResult(0);               
  auto new_inst_2 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "h", llvm::makeArrayRef({inputQubit}),
                      llvm::None);
  inputQubit =  new_inst_2.getResult(0);
  auto new_inst_3 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "s", llvm::makeArrayRef({inputQubit}),
                      llvm::None);
  op.getResult(0).replaceAllUsesWith(*new_inst_3.result_begin());
}

 // {"ch", R"#(gate ch a,b {s b; h b; t b; cx a,b; tdg b; h b; sdg b;})#"},
void decompose_ch_gate(mlir::quantum::ValueSemanticsInstOp &op,std::vector<mlir::quantum::ValueSemanticsInstOp> &deadOps){
  deadOps.emplace_back(op);
  mlir::OpBuilder rewriter(op);
  rewriter.setInsertionPointAfter(op);

  std::vector<mlir::Value> param;
  mlir::Value inputQubit_0 = op.getOperand(0);
  mlir::Value inputQubit_1 = op.getOperand(1);

  mlir::Type qubit_type = inputQubit_0.getType();

  auto new_inst_1 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "s", llvm::makeArrayRef({inputQubit_1}),
                      llvm::None);
  inputQubit_1 =  new_inst_1.getResult(0);               
  auto new_inst_2 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "h", llvm::makeArrayRef({inputQubit_1}),
                      llvm::None);
  inputQubit_1 =  new_inst_2.getResult(0);
  auto new_inst_3 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "t", llvm::makeArrayRef({inputQubit_1}),
                      llvm::None);
  inputQubit_1 =  new_inst_3.getResult(0);
  auto new_inst_4 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type,qubit_type}),
                      "cx", llvm::makeArrayRef({inputQubit_0,inputQubit_1}),
                      llvm::None);                
  inputQubit_1 =  new_inst_4.getResult(1);
  auto new_inst_5 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "tdg", llvm::makeArrayRef({inputQubit_1}),
                      llvm::None);
  inputQubit_1 =  new_inst_5.getResult(0);               
  auto new_inst_6 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "h", llvm::makeArrayRef({inputQubit_1}),
                      llvm::None);
  inputQubit_1 =  new_inst_6.getResult(0);
  auto new_inst_7 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "sdg", llvm::makeArrayRef({inputQubit_1}),
                      llvm::None);
  op.getResult(0).replaceAllUsesWith(new_inst_4.getResult(0));
  op.getResult(1).replaceAllUsesWith(new_inst_7.getResult(0));
}

 // {"cy", R"#(gate cy a,b {sdg b; cx a,b; s b;})#"},
void decompose_cy_gate(mlir::quantum::ValueSemanticsInstOp &op,std::vector<mlir::quantum::ValueSemanticsInstOp> &deadOps){
  deadOps.emplace_back(op);
  mlir::OpBuilder rewriter(op);
  rewriter.setInsertionPointAfter(op);

  std::vector<mlir::Value> param;
  mlir::Value inputQubit_0 = op.getOperand(0);
  mlir::Value inputQubit_1 = op.getOperand(1);

  mlir::Type qubit_type = inputQubit_0.getType();

  auto new_inst_1 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "sdg", llvm::makeArrayRef({inputQubit_1}),
                      llvm::None);
  inputQubit_1 =  new_inst_1.getResult(0);               
  
  auto new_inst_2 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type,qubit_type}),
                      "cx", llvm::makeArrayRef({inputQubit_0,inputQubit_1}),
                      llvm::None);                
  inputQubit_1 =  new_inst_2.getResult(1);
  auto new_inst_3 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "s", llvm::makeArrayRef({inputQubit_1}),
                      llvm::None);
  op.getResult(0).replaceAllUsesWith(new_inst_2.getResult(0));
  op.getResult(1).replaceAllUsesWith(new_inst_3.getResult(0));
}

 // {"cu3", R"#(gate cu3(theta,phi,lambda) c,t { u3(0,0,(lambda+phi)/2) c, u3(0,0,(lambda-phi)/2) t, cx c,t; u3(-theta/2,0,-(phi+lambda)/2) t, cx c,t; u3(theta/2,phi,0) t;})#"}
void decompose_cu3_gate(mlir::quantum::ValueSemanticsInstOp &op,std::vector<mlir::quantum::ValueSemanticsInstOp> &deadOps){
  deadOps.emplace_back(op);
  mlir::OpBuilder rewriter(op);
  rewriter.setInsertionPointAfter(op);

  std::vector<mlir::Value> param;
  mlir::Value inputQubit_0 = op.getOperand(0);
  mlir::Value inputQubit_1 = op.getOperand(1);
  mlir::Value theta_1 = op.getOperand(2);
  mlir::Value theta_2 = op.getOperand(3);
  mlir::Value theta_3 = op.getOperand(4);
  auto theta = qllvm::OP::tryGetConstAngle(theta_1);
  auto phi = qllvm::OP::tryGetConstAngle(theta_2);
  auto lambda = qllvm::OP::tryGetConstAngle(theta_3);
  mlir::Value phi_lambda_v_1 = rewriter.create<mlir::ConstantOp>(
                    op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), (phi+lambda)/2.0));

  mlir::Value theta_zero = rewriter.create<mlir::ConstantOp>(
                    op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), 0));
  
  mlir::Value phi_lambda_v_2 = rewriter.create<mlir::ConstantOp>(
                      op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), (lambda-phi)/2.0));

  mlir::Value phi_lambda_v_3 = rewriter.create<mlir::ConstantOp>(
                    op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), -1*(lambda+phi)/2.0));
  mlir::Value theta_v = rewriter.create<mlir::ConstantOp>(
                    op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), -1*(theta)/2.0));
  
  mlir::Type qubit_type = inputQubit_0.getType();

  param.clear();                 
  param.emplace_back(theta_zero);
  param.emplace_back(theta_zero);
  param.emplace_back(phi_lambda_v_1);
  
  // u3(0,0,(lambda+phi)/2) c;
  auto new_inst_1 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "u3", llvm::makeArrayRef({inputQubit_0}),
                      llvm::makeArrayRef(param));
 
  inputQubit_0 = new_inst_1.getResult(0);

  // u3(0,0,(lambda-phi)/2) t;
  param.clear();
  param.emplace_back(theta_zero);
  param.emplace_back(theta_zero);
  param.emplace_back(phi_lambda_v_2);
  auto new_inst_2 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "u3", llvm::makeArrayRef({inputQubit_1}),
                      llvm::makeArrayRef(param));
  
  inputQubit_1 = new_inst_2.getResult(0);
  // cx c,t; 
  auto new_inst_3 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type,qubit_type}),
                      "cx", llvm::makeArrayRef({inputQubit_0,inputQubit_1}),
                      llvm::None);
  inputQubit_0 = new_inst_3.getResult(0);
  inputQubit_1 = new_inst_3.getResult(1);
  
  // u3(-theta/2,0,-(phi+lambda)/2) t
  param.clear();
  param.emplace_back(theta_v);
  param.emplace_back(theta_zero);
  param.emplace_back(phi_lambda_v_3);
  
  auto new_inst_4 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "u3", llvm::makeArrayRef({inputQubit_1}),
                      llvm::makeArrayRef(param));
  inputQubit_1 = new_inst_4.getResult(0);
  
  // cx c,t; 
  auto new_inst_5 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type,qubit_type}),
                      "cx", llvm::makeArrayRef({inputQubit_0,inputQubit_1}),
                      llvm::None);
  inputQubit_0 = new_inst_5.getResult(0);
  inputQubit_1 = new_inst_5.getResult(1);

  mlir::Value theta_2_v = rewriter.create<mlir::ConstantOp>(
                    op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), theta/2.0));
  
  param.clear();
  param.emplace_back(theta_2_v);
  param.emplace_back(theta_2);
  param.emplace_back(theta_zero);
  // u3(theta/2,phi,0) t;
  auto new_inst_6 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "u3", llvm::makeArrayRef({inputQubit_1}),
                      llvm::makeArrayRef(param));
  inputQubit_1 = new_inst_6.getResult(0);

  op.getResult(0).replaceAllUsesWith(inputQubit_0);
  op.getResult(1).replaceAllUsesWith(inputQubit_1);
}

 // {"cu1",R"#(gate cu1(lambda) a,b {u3(0,0,lambda) a; cx a,b; u3(0,0,-lambda/2) b; cx a,b; u3(0,0,lambda/2) b;})#"},
//  #      ┌─────────┐
//  # q_0: ┤ U1(λ/2) ├──■────────────────■─────────────
//  #      └─────────┘┌─┴─┐┌──────────┐┌─┴─┐┌─────────┐
//  # q_1: ───────────┤ X ├┤ U1(-λ/2) ├┤ X ├┤ U1(λ/2) ├
//  #                 └───┘└──────────┘└───┘└─────────┘
 void decompose_cu1_gate(mlir::quantum::ValueSemanticsInstOp &op,std::vector<mlir::quantum::ValueSemanticsInstOp> &deadOps){
  deadOps.emplace_back(op);
  mlir::OpBuilder rewriter(op);
  rewriter.setInsertionPointAfter(op);

  std::vector<mlir::Value> param;
  mlir::Value inputQubit_0 = op.getOperand(0);
  mlir::Value inputQubit_1 = op.getOperand(1);
  mlir::Value theta_1 = op.getOperand(2);
  mlir::Type qubit_type = inputQubit_0.getType();

  auto lambda = qllvm::OP::tryGetConstAngle(theta_1);

  mlir::Value lambda_v_2 = rewriter.create<mlir::ConstantOp>(
                    op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), lambda/2.0));

  mlir::Value neg_lambda_v_2 = rewriter.create<mlir::ConstantOp>(
                    op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), -1*lambda/2.0));

  mlir::Value theta_zero = rewriter.create<mlir::ConstantOp>(
                    op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), 0));

  param.emplace_back(theta_zero);
  param.emplace_back(theta_zero);
  param.emplace_back(lambda_v_2);
  
  auto new_inst_1 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "u3", llvm::makeArrayRef({inputQubit_0}),
                      llvm::makeArrayRef(param));
  param.clear();
  inputQubit_0 = new_inst_1.getResult(0);

  auto new_inst_2 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type,qubit_type}),
                      "cx", llvm::makeArrayRef({inputQubit_0,inputQubit_1}),
                      llvm::None);
  inputQubit_1 = new_inst_2.getResult(1);
  inputQubit_0 = new_inst_2.getResult(0);

  param.emplace_back(theta_zero);
  param.emplace_back(theta_zero);
  param.emplace_back(neg_lambda_v_2);
  auto new_inst_3 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "u3", llvm::makeArrayRef({inputQubit_1}),
                      llvm::makeArrayRef(param));
  inputQubit_1 = new_inst_3.getResult(0);
  param.clear();

  auto new_inst_4 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type,qubit_type}),
                      "cx", llvm::makeArrayRef({inputQubit_0,inputQubit_1}),
                      llvm::None);
  inputQubit_1 = new_inst_4.getResult(1);

  param.emplace_back(theta_zero);
  param.emplace_back(theta_zero);
  param.emplace_back(lambda_v_2);
  
  auto new_inst_5 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "u3", llvm::makeArrayRef({inputQubit_1}),
                      llvm::makeArrayRef(param));

  op.getResult(0).replaceAllUsesWith(new_inst_4.getResult(0));
  op.getResult(1).replaceAllUsesWith(new_inst_5.getResult(0));
}
//      ┌────────────┐
// q_0: ┤ U(ϴ,φ,λ,γ) ├
//      └─────┬──────┘
// q_1: ──────■───────
// #          ┌──────┐    ┌──────────────┐
// # q_0: ────┤ P(γ) ├────┤ P(λ/2 + φ/2) ├──■────────────────────────────■────────────────
// #      ┌───┴──────┴───┐└──────────────┘┌─┴─┐┌──────────────────────┐┌─┴─┐┌────────────┐
// # q_1: ┤ P(λ/2 - φ/2) ├────────────────┤ X ├┤ U(-0/2,0.0,-λ/2 - φ/2) ├┤ X ├┤ U(0/2,φ,0.0) ├
// #      └──────────────┘                └───┘└──────────────────────┘└───┘└────────────┘
 // {"cu", R"#(gate cu(theta,phi,lambda,gamma) c,t { rz(gamma) c; rz((lambda+phi)/2) c; rz((lambda-phi)/2) t; cx c,t; u3(-theta/2,0,-(phi+lambda)/2) t; cx c,t; u3(theta/2,phi,0) t;})#"}, 
void decompose_cu_gate(mlir::quantum::ValueSemanticsInstOp &op,std::vector<mlir::quantum::ValueSemanticsInstOp> &deadOps){
  deadOps.emplace_back(op);
  mlir::OpBuilder rewriter(op);
  rewriter.setInsertionPointAfter(op);

  std::vector<mlir::Value> param;
  mlir::Value inputQubit_0 = op.getOperand(0);
  mlir::Value inputQubit_1 = op.getOperand(1);
  mlir::Value theta_1 = op.getOperand(2);
  mlir::Value theta_2 = op.getOperand(3);
  mlir::Value theta_3 = op.getOperand(4);
  mlir::Value theta_4 = op.getOperand(5);
  auto theta = qllvm::OP::tryGetConstAngle(theta_1);
  auto phi = qllvm::OP::tryGetConstAngle(theta_2);
  auto lambda = qllvm::OP::tryGetConstAngle(theta_3);

  mlir::Value phi_lambda_v_1 = rewriter.create<mlir::ConstantOp>(
                    op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), (phi+lambda)/2.0));
  mlir::Value phi_lambda_v_2 = rewriter.create<mlir::ConstantOp>(
                    op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), (lambda-phi)/2.0));
  mlir::Value phi_lambda_v_3 = rewriter.create<mlir::ConstantOp>(
                    op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), -lambda/2.0-phi/2.0));
  mlir::Value theta_v = rewriter.create<mlir::ConstantOp>(
                    op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), -1*(theta)/2.0));
  mlir::Value theta_v_p = rewriter.create<mlir::ConstantOp>(
                    op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), (theta)/2.0));
  mlir::Value zero_v = rewriter.create<mlir::ConstantOp>(
                    op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), 0.0));
  mlir::Type qubit_type = inputQubit_0.getType();
  // rz(gamma) c;
  auto new_inst_1 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                  op.getLoc(), llvm::makeArrayRef({qubit_type}),
                  "rz", llvm::makeArrayRef({inputQubit_0}),
                  llvm::makeArrayRef({theta_4}));
  inputQubit_0 = new_inst_1.getResult(0);

   // rz((lambda+phi)/2 ) c; 
  
  auto new_inst_2 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "rz", llvm::makeArrayRef({inputQubit_0}),
                      llvm::makeArrayRef({phi_lambda_v_1}));
  inputQubit_0 = new_inst_2.getResult(0);

   // rz((lambda-phi)/2) t; 
  auto new_inst_3 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "rz", llvm::makeArrayRef({inputQubit_1}),
                      llvm::makeArrayRef({phi_lambda_v_2}));
  inputQubit_1 = new_inst_3.getResult(0);

   // cx c,t; 
  auto new_inst_4 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type,qubit_type}),
                      "cx", llvm::makeArrayRef({inputQubit_0,inputQubit_1}),
                      llvm::None);
  inputQubit_0 = new_inst_4.getResult(0);
  inputQubit_1 = new_inst_4.getResult(1);

   // u3(-theta/2,0,-(phi+lambda)/2) t; 
  param.clear();
  param.emplace_back(theta_v);
  param.emplace_back(zero_v);
  param.emplace_back(phi_lambda_v_3);
  auto new_inst_5 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "u3", llvm::makeArrayRef({inputQubit_1}),
                      llvm::makeArrayRef(param));
  inputQubit_1 = new_inst_5.getResult(0);

   // cx c,t; 
  auto new_inst_6 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type,qubit_type}),
                      "cx", llvm::makeArrayRef({inputQubit_0,inputQubit_1}),
                      llvm::None);

  inputQubit_0 = new_inst_6.getResult(0);
  inputQubit_1 = new_inst_6.getResult(1);
  param.clear();
  param.emplace_back(theta_v_p);
  param.emplace_back(theta_2);
  param.emplace_back(zero_v);
  
   // u3(theta/2,phi,0) t;})
  auto new_inst_7 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "u3", llvm::makeArrayRef({inputQubit_1}),
                      llvm::makeArrayRef(param));
  inputQubit_1 = new_inst_7.getResult(0);
  op.getResult(0).replaceAllUsesWith(inputQubit_0);
  op.getResult(1).replaceAllUsesWith(inputQubit_1);

}

 // {"csx", R"#(gate csx a,b { h b; u3(0,0,pi/2) a; cx a,b; u3(0,0,-pi/4) b; cx a,b; u3(0,0,pi/4) b; h b; })#"}
void decompose_csx_gate(mlir::quantum::ValueSemanticsInstOp &op,std::vector<mlir::quantum::ValueSemanticsInstOp> &deadOps){
  deadOps.emplace_back(op);
  mlir::OpBuilder rewriter(op);
  rewriter.setInsertionPointAfter(op);

  std::vector<mlir::Value> param;
  mlir::Value inputQubit_0 = op.getOperand(0);
  mlir::Value inputQubit_1 = op.getOperand(1);

  mlir::Value PI_2_V = rewriter.create<mlir::ConstantOp>(
                    op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), M_PI_2));
  mlir::Value PI_4_V = rewriter.create<mlir::ConstantOp>(
                    op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), M_PI_4));
  mlir::Value neg_PI_4_V = rewriter.create<mlir::ConstantOp>(
                    op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), -1*M_PI_4));

  mlir::Value theta_zero = rewriter.create<mlir::ConstantOp>(
                    op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), 0));


  mlir::Type qubit_type = inputQubit_0.getType();
   // h b; 
  auto new_inst_1 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "h", llvm::makeArrayRef({inputQubit_1}),
                      llvm::None);
  inputQubit_1 = new_inst_1.getResult(0);
  
   // u3(0,0,pi/2) a;
  param.emplace_back(theta_zero);
  param.emplace_back(theta_zero);
  param.emplace_back(PI_2_V);
  auto new_inst_2 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "u3", llvm::makeArrayRef({inputQubit_0}),
                      llvm::makeArrayRef(param));
  param.clear();
  inputQubit_0 = new_inst_2.getResult(0);

   // cx a,b;  
  auto new_inst_3 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type,qubit_type}),
                      "cx", llvm::makeArrayRef({inputQubit_0,inputQubit_1}),
                      llvm::None);
  inputQubit_0 = new_inst_3.getResult(0);
  inputQubit_1 = new_inst_3.getResult(1);

   // u3(0,0,-pi/4) b; 
  param.emplace_back(theta_zero);
  param.emplace_back(theta_zero);
  param.emplace_back(neg_PI_4_V);

  auto new_inst_4 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "u3", llvm::makeArrayRef({inputQubit_1}),
                      llvm::makeArrayRef(param));
  inputQubit_1 = new_inst_4.getResult(0);
  param.clear();
  
   // cx a,b; 
  auto new_inst_5 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type,qubit_type}),
                      "cx", llvm::makeArrayRef({inputQubit_0,inputQubit_1}),
                      llvm::None);
  inputQubit_0 = new_inst_5.getResult(0);
  inputQubit_1 = new_inst_5.getResult(1);
  
   // u3(0,0,pi/4) b; 
  param.emplace_back(theta_zero);
  param.emplace_back(theta_zero);
  param.emplace_back(PI_4_V);
  auto new_inst_6 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "u3", llvm::makeArrayRef({inputQubit_1}),
                      llvm::makeArrayRef(param));
  param.clear();
  inputQubit_1 = new_inst_6.getResult(0);

   // h b;
  auto new_inst_7 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "h", llvm::makeArrayRef({inputQubit_1}),
                      llvm::None);

  op.getResult(0).replaceAllUsesWith(new_inst_5.getResult(0));
  op.getResult(1).replaceAllUsesWith(new_inst_7.getResult(0));

}

// gate ccx a,b,c
//         {
//         h c; cx b,c; tdg c; cx a,c;
//         t c; cx b,c; tdg c; cx a,c;
//         t b; t c; h c; cx a,b;
//         t a; tdg b; cx a,b;}

void decompose_ccx_gate(mlir::quantum::ValueSemanticsInstOp &op,std::vector<mlir::quantum::ValueSemanticsInstOp> &deadOps){
  deadOps.emplace_back(op);

  mlir::OpBuilder rewriter(op);
  rewriter.setInsertionPointAfter(op);

  std::vector<mlir::Value> param;
  mlir::Value inputQubit_0 = op.getOperand(0);
  mlir::Value inputQubit_1 = op.getOperand(1);
  mlir::Value inputQubit_2 = op.getOperand(2);

  mlir::Type qubit_type = inputQubit_0.getType();
   // h c; 
  auto new_inst_1 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "h", llvm::makeArrayRef({inputQubit_2}),
                      llvm::None);
  inputQubit_2 =  new_inst_1.getResult(0);

   // cx b,c; 
  auto new_inst_2 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type,qubit_type}),
                      "cx", llvm::makeArrayRef({inputQubit_1,inputQubit_2}),
                      llvm::None);                
  inputQubit_1 =  new_inst_2.getResult(0);
  inputQubit_2 =  new_inst_2.getResult(1);
  
   // tdg c; 
  auto new_inst_3 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "tdg", llvm::makeArrayRef({inputQubit_2}),
                      llvm::None);
  inputQubit_2 =  new_inst_3.getResult(0);

   // cx a,c;
  auto new_inst_4 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type,qubit_type}),
                      "cx", llvm::makeArrayRef({inputQubit_0,inputQubit_2}),
                      llvm::None);
  inputQubit_0 =  new_inst_4.getResult(0);
  inputQubit_2 =  new_inst_4.getResult(1);
  
   // t c; 
  auto new_inst_5 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "t", llvm::makeArrayRef({inputQubit_2}),
                      llvm::None);
  inputQubit_2 =  new_inst_5.getResult(0);

   // cx b,c; 
  auto new_inst_6 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type,qubit_type}),
                      "cx", llvm::makeArrayRef({inputQubit_1,inputQubit_2}),
                      llvm::None);
  inputQubit_1 =  new_inst_6.getResult(0);
  inputQubit_2 =  new_inst_6.getResult(1);

   // tdg c; 
  auto new_inst_7 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "tdg", llvm::makeArrayRef({inputQubit_2}),
                      llvm::None);
  inputQubit_2 =  new_inst_7.getResult(0);
  
   // cx a,c;
  auto new_inst_8 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type,qubit_type}),
                      "cx", llvm::makeArrayRef({inputQubit_0,inputQubit_2}),
                      llvm::None);
  inputQubit_0 =  new_inst_8.getResult(0);
  inputQubit_2 =  new_inst_8.getResult(1);
  
   // t b; 
  auto new_inst_9 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "t", llvm::makeArrayRef({inputQubit_1}),
                      llvm::None);
  inputQubit_1 =  new_inst_9.getResult(0);
  
   // t c; 
  auto new_inst_10 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "t", llvm::makeArrayRef({inputQubit_2}),
                      llvm::None);
  inputQubit_2 =  new_inst_10.getResult(0);

   // h c; 
  auto new_inst_11 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "h", llvm::makeArrayRef({inputQubit_2}),
                      llvm::None);
  inputQubit_2 =  new_inst_11.getResult(0);

   // cx a,b; 
  auto new_inst_12 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type,qubit_type}),
                      "cx", llvm::makeArrayRef({inputQubit_0,inputQubit_1}),
                      llvm::None);
  inputQubit_0 =  new_inst_12.getResult(0);
  inputQubit_1 =  new_inst_12.getResult(1);
   // t a; 
  auto new_inst_13 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "t", llvm::makeArrayRef({inputQubit_0}),
                      llvm::None);
  inputQubit_0 =  new_inst_13.getResult(0);

   // tdg b;
  auto new_inst_14 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "tdg", llvm::makeArrayRef({inputQubit_1}),
                      llvm::None);
  inputQubit_1 =  new_inst_14.getResult(0);

   // cx a,b;
  auto new_inst_15 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type,qubit_type}),
                      "cx", llvm::makeArrayRef({inputQubit_0,inputQubit_1}),
                      llvm::None);
  inputQubit_0 =  new_inst_15.getResult(0);
  inputQubit_1 =  new_inst_15.getResult(1);

  op.getResult(0).replaceAllUsesWith(inputQubit_0);
  op.getResult(1).replaceAllUsesWith(inputQubit_1);
  op.getResult(2).replaceAllUsesWith(inputQubit_2);
}

 // {"cswap", R"#(gate cswap a,b,c { cx c,b; h c; cx b,c; tdg c; cx a,c; t c; cx b,c; tdg c; cx a,c; t b; t c; h c; cx a,b; t a; tdg b; cx a,b; cx c,b;})#"}
void decompose_cswap_gate(mlir::quantum::ValueSemanticsInstOp &op,std::vector<mlir::quantum::ValueSemanticsInstOp> &deadOps){
  deadOps.emplace_back(op);
  mlir::OpBuilder rewriter(op);
  rewriter.setInsertionPointAfter(op);

  std::vector<mlir::Value> param;
  mlir::Value inputQubit_0 = op.getOperand(0);
  mlir::Value inputQubit_1 = op.getOperand(1);
  mlir::Value inputQubit_2 = op.getOperand(2);

  mlir::Type qubit_type = inputQubit_0.getType();
   // cx c,b;  
  auto new_inst_1 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type,qubit_type}),
                      "cx", llvm::makeArrayRef({inputQubit_2,inputQubit_1}),
                      llvm::None);
  inputQubit_1 =  new_inst_1.getResult(1);    
  inputQubit_2 =  new_inst_1.getResult(0);
   // h c; 
  auto new_inst_2 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "h", llvm::makeArrayRef({inputQubit_2}),
                      llvm::None);                
  inputQubit_2 =  new_inst_2.getResult(0);
  
   // cx b,c;  
  auto new_inst_3 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type,qubit_type}),
                      "cx", llvm::makeArrayRef({inputQubit_1,inputQubit_2}),
                      llvm::None);
  inputQubit_1 =  new_inst_3.getResult(0);
  inputQubit_2 =  new_inst_3.getResult(1);

   // tdg c; 
  auto new_inst_4 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "tdg", llvm::makeArrayRef({inputQubit_2}),
                      llvm::None);
  inputQubit_2 =  new_inst_4.getResult(0);
  
   // cx a,c; 
  auto new_inst_5 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type,qubit_type}),
                      "cx", llvm::makeArrayRef({inputQubit_0,inputQubit_2}),
                      llvm::None);
  inputQubit_0 =  new_inst_5.getResult(0);
  inputQubit_2 =  new_inst_5.getResult(1);

   // t c; 
  auto new_inst_6 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "t", llvm::makeArrayRef({inputQubit_2}),
                      llvm::None);
  inputQubit_2 =  new_inst_6.getResult(0);

   // cx b,c; 
  auto new_inst_7 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type,qubit_type}),
                      "cx", llvm::makeArrayRef({inputQubit_1,inputQubit_2}),
                      llvm::None);
  inputQubit_1 =  new_inst_7.getResult(0);
  inputQubit_2 =  new_inst_7.getResult(1);
  
   // tdg c; 
  auto new_inst_8 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "tdg", llvm::makeArrayRef({inputQubit_2}),
                      llvm::None);
  inputQubit_2 =  new_inst_8.getResult(0);
   // cx a,c; 
  auto new_inst_9 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type,qubit_type}),
                      "cx", llvm::makeArrayRef({inputQubit_0,inputQubit_2}),
                      llvm::None);
  inputQubit_0 =  new_inst_9.getResult(0);
  inputQubit_2 =  new_inst_9.getResult(1);
   // t b;  
  auto new_inst_10 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "t", llvm::makeArrayRef({inputQubit_1}),
                      llvm::None);
  inputQubit_1 =  new_inst_10.getResult(0);
   // t c; 
  auto new_inst_11 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "t", llvm::makeArrayRef({inputQubit_2}),
                      llvm::None);
  inputQubit_2 =  new_inst_11.getResult(0);
   // h c; 
  auto new_inst_12 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "h", llvm::makeArrayRef({inputQubit_2}),
                      llvm::None);
  inputQubit_2 =  new_inst_12.getResult(0);
   // cx a,b;  
  auto new_inst_13 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type,qubit_type}),
                      "cx", llvm::makeArrayRef({inputQubit_0,inputQubit_1}),
                      llvm::None);
  inputQubit_0 =  new_inst_13.getResult(0);
  inputQubit_1 =  new_inst_13.getResult(1);
   // t a; 
  auto new_inst_14 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "t", llvm::makeArrayRef({inputQubit_0}),
                      llvm::None);
  inputQubit_0 =  new_inst_14.getResult(0);
   // tdg b; 
  auto new_inst_15 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "tdg", llvm::makeArrayRef({inputQubit_1}),
                      llvm::None);
  inputQubit_1 =  new_inst_15.getResult(0);
   // cx a,b;
  auto new_inst_16 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type,qubit_type}),
                      "cx", llvm::makeArrayRef({inputQubit_0,inputQubit_1}),
                      llvm::None);
  inputQubit_0 =  new_inst_16.getResult(0);
  inputQubit_1 =  new_inst_16.getResult(1);
   //  cx c,b;
  auto new_inst_17 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type,qubit_type}),
                      "cx", llvm::makeArrayRef({inputQubit_2,inputQubit_1}),
                      llvm::None);
  inputQubit_1 =  new_inst_17.getResult(1);
  inputQubit_2 =  new_inst_17.getResult(0);
  op.getResult(0).replaceAllUsesWith(inputQubit_0);
  op.getResult(1).replaceAllUsesWith(inputQubit_1);
  op.getResult(2).replaceAllUsesWith(inputQubit_2);

}

 // {"crx", R"#(gate crx(lambda) a,b { u3(0,0,pi/2) b; cx a,b; u3(-lambda/2,0,0) b; cx a,b; u3(lambda/2,-pi/2,0) b;})#"}

 void decompose_crx_gate(mlir::quantum::ValueSemanticsInstOp &op,std::vector<mlir::quantum::ValueSemanticsInstOp> &deadOps){
  deadOps.emplace_back(op);
  mlir::OpBuilder rewriter(op);
  rewriter.setInsertionPointAfter(op);

  std::vector<mlir::Value> param;
  mlir::Value inputQubit_0 = op.getOperand(0);
  mlir::Value inputQubit_1 = op.getOperand(1);
  mlir::Value theta = op.getOperand(2);
  auto lambda = qllvm::OP::tryGetConstAngle(theta);

  mlir::Value PI_2_V = rewriter.create<mlir::ConstantOp>(
                    op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), M_PI_2));
  mlir::Value neg_PI_2_V = rewriter.create<mlir::ConstantOp>(
                    op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), -1*M_PI_2));
  mlir::Value lambda_h_v = rewriter.create<mlir::ConstantOp>(
                    op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), lambda/2.0));
  mlir::Value neg_lambda_h_v = rewriter.create<mlir::ConstantOp>(
                    op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), -1*lambda/2.0));

  mlir::Value theta_zero = rewriter.create<mlir::ConstantOp>(
                    op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), 0));


  mlir::Type qubit_type = inputQubit_0.getType();

   // u3(0,0,pi/2) b;
  param.emplace_back(theta_zero);
  param.emplace_back(theta_zero);
  param.emplace_back(PI_2_V);
  auto new_inst_1 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "u3", llvm::makeArrayRef({inputQubit_1}),
                      llvm::makeArrayRef(param));
  param.clear();
  inputQubit_1 = new_inst_1.getResult(0);

   // cx a,b;  
  auto new_inst_2 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type,qubit_type}),
                      "cx", llvm::makeArrayRef({inputQubit_0,inputQubit_1}),
                      llvm::None);
  inputQubit_0 = new_inst_2.getResult(0);
  inputQubit_1 = new_inst_2.getResult(1);

   //u3(-lambda/2,0,0) b; 
  param.emplace_back(neg_lambda_h_v);
  param.emplace_back(theta_zero);
  param.emplace_back(theta_zero);

  auto new_inst_3 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "u3", llvm::makeArrayRef({inputQubit_1}),
                      llvm::makeArrayRef(param));
  inputQubit_1 = new_inst_3.getResult(0);
  param.clear();
  
   // cx a,b; 
  auto new_inst_4 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type,qubit_type}),
                      "cx", llvm::makeArrayRef({inputQubit_0,inputQubit_1}),
                      llvm::None);
  inputQubit_0 = new_inst_4.getResult(0);
  inputQubit_1 = new_inst_4.getResult(1);
  
   //u3(lambda/2,-pi/2,0) b;
  param.emplace_back(lambda_h_v);
  param.emplace_back(neg_PI_2_V);
  param.emplace_back(theta_zero);
  
  auto new_inst_5 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "u3", llvm::makeArrayRef({inputQubit_1}),
                      llvm::makeArrayRef(param));
  param.clear();
  inputQubit_1 = new_inst_5.getResult(0);

  op.getResult(0).replaceAllUsesWith(inputQubit_0);
  op.getResult(1).replaceAllUsesWith(inputQubit_1);
}

 // {"cry"

//  # q_0: ─────────────■───────────────■──
//  #      ┌─────────┐┌─┴─┐┌─────────┐┌─┴─┐
//  # q_1: ┤ Ry(λ/2) ├┤ X ├┤ Ry(-λ/2) ├┤ X ├
//  #      └─────────┘└───┘└─────────┘└───┘

void decompose_cry_gate(mlir::quantum::ValueSemanticsInstOp &op,std::vector<mlir::quantum::ValueSemanticsInstOp> &deadOps){
  deadOps.emplace_back(op);

  mlir::OpBuilder rewriter(op);
  rewriter.setInsertionPointAfter(op);

  std::vector<mlir::Value> param;
  mlir::Value inputQubit_0 = op.getOperand(0);
  mlir::Value inputQubit_1 = op.getOperand(1);
  mlir::Value theta = op.getOperand(2);

  auto lambda = qllvm::OP::tryGetConstAngle(theta);

  mlir::Value lambda_h_v = rewriter.create<mlir::ConstantOp>(
                    op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), lambda/2.0));
  mlir::Value neg_lambda_h_v = rewriter.create<mlir::ConstantOp>(
                      op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), -1.0*lambda/2.0));
  mlir::Type qubit_type = inputQubit_0.getType();

   // ('RY', angle / 2, target)
  auto new_inst_1 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "ry", llvm::makeArrayRef({inputQubit_1}),
                      llvm::makeArrayRef({lambda_h_v}));
  inputQubit_1 = new_inst_1.getResult(0);

   // cx a,b;  
  auto new_inst_2 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type,qubit_type}),
                      "cx", llvm::makeArrayRef({inputQubit_0,inputQubit_1}),
                      llvm::None);
  inputQubit_0 = new_inst_2.getResult(0);
  inputQubit_1 = new_inst_2.getResult(1);

   //('RY', -angle / 2, target),
  auto new_inst_3 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "ry", llvm::makeArrayRef({inputQubit_1}),
                      llvm::makeArrayRef({neg_lambda_h_v}));
  inputQubit_1 = new_inst_3.getResult(0);

  auto new_inst_4 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type,qubit_type}),
                      "cx", llvm::makeArrayRef({inputQubit_0,inputQubit_1}),
                      llvm::None);
  inputQubit_0 = new_inst_4.getResult(0);
  inputQubit_1 = new_inst_4.getResult(1);

  op.getResult(0).replaceAllUsesWith(inputQubit_0);
  op.getResult(1).replaceAllUsesWith(inputQubit_1);
}

 // {"crz"

//  # q_0: ─────────────■────────────────■──
//  #      ┌─────────┐┌─┴─┐┌──────────┐┌─┴─┐
//  # q_1: ┤ Rz(λ/2) ├┤ X ├┤ Rz(-λ/2) ├┤ X ├
//  #      └─────────┘└───┘└──────────┘└───┘
void decompose_crz_gate(mlir::quantum::ValueSemanticsInstOp &op,std::vector<mlir::quantum::ValueSemanticsInstOp> &deadOps){
  deadOps.emplace_back(op);
  mlir::OpBuilder rewriter(op);
  rewriter.setInsertionPointAfter(op);

  std::vector<mlir::Value> param;
  mlir::Value inputQubit_0 = op.getOperand(0);
  mlir::Value inputQubit_1 = op.getOperand(1);
  mlir::Value theta = op.getOperand(2);
  auto lambda = qllvm::OP::tryGetConstAngle(theta);

  mlir::Value lambda_h_v = rewriter.create<mlir::ConstantOp>(
                    op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), lambda/2.0));
  mlir::Value neg_lambda_v = rewriter.create<mlir::ConstantOp>(
                    op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), -1*lambda/2.0));

  mlir::Value theta_zero = rewriter.create<mlir::ConstantOp>(
                    op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), 0));


  mlir::Type qubit_type = inputQubit_0.getType();

   // ('RZ', angle / 2, target)

  auto new_inst_1 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "rz", llvm::makeArrayRef({inputQubit_1}),
                      llvm::makeArrayRef({lambda_h_v}));
  inputQubit_1 = new_inst_1.getResult(0);

   // ('CNOT', control, target)  
  auto new_inst_2 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type,qubit_type}),
                      "cx", llvm::makeArrayRef({inputQubit_0,inputQubit_1}),
                      llvm::None);
  inputQubit_0 = new_inst_2.getResult(0);
  inputQubit_1 = new_inst_2.getResult(1);

   //('RZ', -angle / 2, target),
  auto new_inst_3 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "rz", llvm::makeArrayRef({inputQubit_1}),
                      llvm::makeArrayRef({neg_lambda_v}));
  inputQubit_1 = new_inst_3.getResult(0);
  
   // ('CNOT', control, target),
  auto new_inst_4 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type,qubit_type}),
                      "cx", llvm::makeArrayRef({inputQubit_0,inputQubit_1}),
                      llvm::None);
  inputQubit_0 = new_inst_4.getResult(0);
  inputQubit_1 = new_inst_4.getResult(1);
  
  op.getResult(0).replaceAllUsesWith(inputQubit_0);
  op.getResult(1).replaceAllUsesWith(inputQubit_1);
}

         // #      ┌───┐                   ┌───┐
         // # q_0: ┤ H ├──■─────────────■──┤ H ├
         // #      ├───┤┌─┴─┐┌───────┐┌─┴─┐├───┤
         // # q_1: ┤ H ├┤ X ├┤ Rz(0) ├┤ X ├┤ H ├
         // #      └───┘└───┘└───────┘└───┘└───┘
void decompose_rxx_gate(mlir::quantum::ValueSemanticsInstOp &op,std::vector<mlir::quantum::ValueSemanticsInstOp> &deadOps){
  deadOps.emplace_back(op);
  mlir::OpBuilder rewriter(op);
  rewriter.setInsertionPointAfter(op);

  std::vector<mlir::Value> param;
  mlir::Value inputQubit_0 = op.getOperand(0);
  mlir::Value inputQubit_1 = op.getOperand(1);
  mlir::Value theta = op.getOperand(2);
  auto theta_d = qllvm::OP::tryGetConstAngle(theta);

  mlir::Type qubit_type = inputQubit_0.getType();

   // h a; 

  auto new_inst_1 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "h", llvm::makeArrayRef({inputQubit_0}),
                      llvm::None);
  inputQubit_0 = new_inst_1.getResult(0);
   //h b; 
  auto new_inst_2 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "h", llvm::makeArrayRef({inputQubit_1}),
                      llvm::None);
  inputQubit_1 = new_inst_2.getResult(0);

   // cx a,b; 
  auto new_inst_3 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type,qubit_type}),
                      "cx", llvm::makeArrayRef({inputQubit_0,inputQubit_1}),
                      llvm::None);
  inputQubit_0 = new_inst_3.getResult(0);
  inputQubit_1 = new_inst_3.getResult(1);
   //rz(theta) b;

  auto new_inst_4 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "rz", llvm::makeArrayRef({inputQubit_1}),
                      llvm::makeArrayRef({theta}));
  inputQubit_1 = new_inst_4.getResult(0);

   // cx a,b; 
  auto new_inst_5 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type,qubit_type}),
                      "cx", llvm::makeArrayRef({inputQubit_0,inputQubit_1}),
                      llvm::None);
  inputQubit_0 = new_inst_5.getResult(0);
  inputQubit_1 = new_inst_5.getResult(1);
   // h b; 
  auto new_inst_6 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "h", llvm::makeArrayRef({inputQubit_1}),
                      llvm::None);
  inputQubit_1 = new_inst_6.getResult(0);
   //h a;
  auto new_inst_7 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "h", llvm::makeArrayRef({inputQubit_0}),
                      llvm::None);
  inputQubit_0 = new_inst_7.getResult(0);
  op.getResult(0).replaceAllUsesWith(inputQubit_0);
  op.getResult(1).replaceAllUsesWith(inputQubit_1);
}

 // {"rzz", R"#(gate rzz(theta) a,b { cx a,b; u3(0,0,theta) b; cx a,b;})#"},
void decompose_rzz_gate(mlir::quantum::ValueSemanticsInstOp &op,std::vector<mlir::quantum::ValueSemanticsInstOp> &deadOps){
  deadOps.emplace_back(op);
  mlir::OpBuilder rewriter(op);
  rewriter.setInsertionPointAfter(op);

  std::vector<mlir::Value> param;
  mlir::Value inputQubit_0 = op.getOperand(0);
  mlir::Value inputQubit_1 = op.getOperand(1);

  mlir::Value theta = op.getOperand(2);
  mlir::Value theta_zero = rewriter.create<mlir::ConstantOp>(
                    op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), 0));

  mlir::Type qubit_type = inputQubit_0.getType();

  auto new_inst_1 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type,qubit_type}),
                      "cx", llvm::makeArrayRef({inputQubit_0,inputQubit_1}),
                      llvm::None);
  inputQubit_0 = new_inst_1.getResult(0);
  inputQubit_1 = new_inst_1.getResult(1);
  
  param.emplace_back(theta_zero);
  param.emplace_back(theta_zero);
  param.emplace_back(theta);
  auto new_inst_2 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "u3", llvm::makeArrayRef({inputQubit_1}),
                      llvm::makeArrayRef(param));
  inputQubit_1 = new_inst_2.getResult(0); 

  auto new_inst_3 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type,qubit_type}),
                      "cx", llvm::makeArrayRef({inputQubit_0,inputQubit_1}),
                      llvm::None);
  op.getResult(0).replaceAllUsesWith(new_inst_3.getResult(0));
  op.getResult(1).replaceAllUsesWith(new_inst_3.getResult(1));
}

// gate rccx a,b,c
//         { u2(0,pi) c;
//           u1(pi/4) c;
//           cx b, c;
//           u1(-pi/4) c;
//           cx a, c;
//           u1(pi/4) c;
//           cx b, c;
//           u1(-pi/4) c;
//           u2(0,pi) c;
//         }
void decompose_rccx_gate(mlir::quantum::ValueSemanticsInstOp &op,std::vector<mlir::quantum::ValueSemanticsInstOp> &deadOps){
  deadOps.emplace_back(op);
  mlir::OpBuilder rewriter(op);
  rewriter.setInsertionPointAfter(op);

  std::vector<mlir::Value> param;
  mlir::Value inputQubit_0 = op.getOperand(0);
  mlir::Value inputQubit_1 = op.getOperand(1);
  mlir::Value inputQubit_2 = op.getOperand(2);

  mlir::Value theta_zero = rewriter.create<mlir::ConstantOp>(
                    op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), 0));
  mlir::Value theta_PI = rewriter.create<mlir::ConstantOp>(
                    op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), M_PI));
  mlir::Value theta_PI_4 = rewriter.create<mlir::ConstantOp>(
                    op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), M_PI_4));
  mlir::Value theta_neg_PI_4 = rewriter.create<mlir::ConstantOp>(
                    op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), -1*M_PI_4));
  mlir::Value theta_PI_2 = rewriter.create<mlir::ConstantOp>(
                      op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), M_PI_2));
  mlir::Type qubit_type = inputQubit_0.getType();

   //u2(0,pi) c;
  param.emplace_back(theta_PI_2);
  param.emplace_back(theta_zero);
  param.emplace_back(theta_PI);
  
  auto new_inst_1 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "u3", llvm::makeArrayRef({inputQubit_2}),
                      llvm::makeArrayRef(param));
  param.clear();
  inputQubit_2 = new_inst_1.getResult(0);

   //u1(pi/4) c;

  param.emplace_back(theta_zero);
  param.emplace_back(theta_zero);
  param.emplace_back(theta_PI_4);
  
  auto new_inst_2 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "u3", llvm::makeArrayRef({inputQubit_2}),
                      llvm::makeArrayRef(param));
  param.clear();
  inputQubit_2 = new_inst_2.getResult(0);

    //           cx b, c;

  auto new_inst_3 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type,qubit_type}),
                      "cx", llvm::makeArrayRef({inputQubit_1,inputQubit_2}),
                      llvm::None);
  inputQubit_1 = new_inst_3.getResult(0);
  inputQubit_2 = new_inst_3.getResult(1);

   //           u1(-pi/4) c;

  param.emplace_back(theta_zero);
  param.emplace_back(theta_zero);
  param.emplace_back(theta_neg_PI_4);

  auto new_inst_4 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "u3", llvm::makeArrayRef({inputQubit_2}),
                      llvm::makeArrayRef(param));
  inputQubit_2 = new_inst_4.getResult(0);
  param.clear();
  
   //           cx a, c;

  auto new_inst_5 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type,qubit_type}),
                      "cx", llvm::makeArrayRef({inputQubit_0,inputQubit_2}),
                      llvm::None);
  inputQubit_0 = new_inst_5.getResult(0);
  inputQubit_2 = new_inst_5.getResult(1);
   //           u1(pi/4) c;

  param.emplace_back(theta_zero);
  param.emplace_back(theta_zero);
  param.emplace_back(theta_PI_4);

  auto new_inst_6 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "u3", llvm::makeArrayRef({inputQubit_2}),
                      llvm::makeArrayRef(param));
  inputQubit_2 = new_inst_6.getResult(0);
  param.clear();
   //           cx b, c;

  auto new_inst_7 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type,qubit_type}),
                      "cx", llvm::makeArrayRef({inputQubit_1,inputQubit_2}),
                      llvm::None);
  inputQubit_1 = new_inst_7.getResult(0);
  inputQubit_2 = new_inst_7.getResult(1);

   //           u1(-pi/4) c;

  param.emplace_back(theta_zero);
  param.emplace_back(theta_zero);
  param.emplace_back(theta_neg_PI_4);

  auto new_inst_8 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "u3", llvm::makeArrayRef({inputQubit_2}),
                      llvm::makeArrayRef(param));
  inputQubit_2 = new_inst_8.getResult(0);
  param.clear();
  //           u2(0,pi) c; 
  param.emplace_back(theta_PI_2);
  param.emplace_back(theta_zero);
  param.emplace_back(theta_PI);

  auto new_inst_9 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "u3", llvm::makeArrayRef({inputQubit_2}),
                      llvm::makeArrayRef(param));
  inputQubit_2 = new_inst_9.getResult(0);

  op.getResult(0).replaceAllUsesWith(inputQubit_0);
  op.getResult(1).replaceAllUsesWith(inputQubit_1);
  op.getResult(2).replaceAllUsesWith(inputQubit_2);
}

 // {"rc3x",R"#(gate rc3x a,b,c,d
 //         {
 //           u3(0,0,5*pi/4) d;
 //           cx c,d;
 //           u3(0,0,3*pi/4) d;
 //           cx a,d;
 //           u3(0,0,pi/4) d;
 //           cx b,d;
 //           u3(0,0,-pi/4) d;
 //           cx a,d;
 //           u3(0,0,pi/4) d;
 //           cx b,d;
 //           u3(0,0,pi) d;
 //           cx c,d;
 //           u3(0,0,3*pi/4) d;
 //         })#"},
void decompose_rc3x_gate(mlir::quantum::ValueSemanticsInstOp &op,std::vector<mlir::quantum::ValueSemanticsInstOp> &deadOps){
  deadOps.emplace_back(op);
  mlir::OpBuilder rewriter(op);
  rewriter.setInsertionPointAfter(op);

  std::vector<mlir::Value> param;
  mlir::Value inputQubit_0 = op.getOperand(0);
  mlir::Value inputQubit_1 = op.getOperand(1);
  mlir::Value inputQubit_2 = op.getOperand(2);
  mlir::Value inputQubit_3 = op.getOperand(3);
  mlir::Type qubit_type = inputQubit_0.getType();
  mlir::Value theta_PI_4 = rewriter.create<mlir::ConstantOp>(
                    op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), M_PI_4));
  mlir::Value theta_neg_PI_4 = rewriter.create<mlir::ConstantOp>(
                    op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), -1*M_PI_4));
  mlir::Value theta_PI = rewriter.create<mlir::ConstantOp>(
                    op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), M_PI));
  mlir::Value theta_PI_5_4 = rewriter.create<mlir::ConstantOp>(
                    op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), 5*M_PI_4));
  mlir::Value theta_PI_3_4 = rewriter.create<mlir::ConstantOp>(
                    op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), 3*M_PI_4));
  mlir::Value theta_thero = rewriter.create<mlir::ConstantOp>(
                    op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), 0));
   //           u3(0,0,5*pi/4) d;
  auto new_inst_1 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "rz", llvm::makeArrayRef({inputQubit_3}),
                      llvm::makeArrayRef({theta_PI_5_4}));
  inputQubit_3 = new_inst_1.getResult(0);
   //           cx c,d;
  auto new_inst_2 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type,qubit_type}),
                      "cx", llvm::makeArrayRef({inputQubit_2,inputQubit_3}),
                      llvm::None);
  inputQubit_2 = new_inst_2.getResult(0);
  inputQubit_3 = new_inst_2.getResult(1);
   //           u3(0,0,3*pi/4) d;
  auto new_inst_3 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "rz", llvm::makeArrayRef({inputQubit_3}),
                      llvm::makeArrayRef({theta_PI_3_4}));
  inputQubit_3 = new_inst_3.getResult(0);
   //           cx a,d;
  auto new_inst_4 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type,qubit_type}),
                      "cx", llvm::makeArrayRef({inputQubit_0,inputQubit_3}),
                      llvm::None);
  inputQubit_0 = new_inst_4.getResult(0);
  inputQubit_3 = new_inst_4.getResult(1);
   //           u3(0,0,pi/4) d;
  auto new_inst_5 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "rz", llvm::makeArrayRef({inputQubit_3}),
                      llvm::makeArrayRef({theta_PI_4}));
  inputQubit_3 = new_inst_5.getResult(0);
   //           cx b,d;
  auto new_inst_6 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type,qubit_type}),
                      "cx", llvm::makeArrayRef({inputQubit_1,inputQubit_3}),
                      llvm::None);
  inputQubit_1 = new_inst_6.getResult(0);
  inputQubit_3 = new_inst_6.getResult(1);
   //           u3(0,0,-pi/4) d;
  auto new_inst_7 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "rz", llvm::makeArrayRef({inputQubit_3}),
                      llvm::makeArrayRef({theta_neg_PI_4}));
  inputQubit_3 = new_inst_7.getResult(0);
   //           cx a,d;
  auto new_inst_8 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type,qubit_type}),
                      "cx", llvm::makeArrayRef({inputQubit_0,inputQubit_3}),
                      llvm::None);
  inputQubit_0 = new_inst_8.getResult(0);
  inputQubit_3 = new_inst_8.getResult(1);
   //           u3(0,0,pi/4) d;
  auto new_inst_9 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "rz", llvm::makeArrayRef({inputQubit_3}),
                      llvm::makeArrayRef({theta_PI_4}));
  inputQubit_3 = new_inst_9.getResult(0);
   //           cx b,d;
  auto new_inst_10 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type,qubit_type}),
                      "cx", llvm::makeArrayRef({inputQubit_1,inputQubit_3}),
                      llvm::None);
  inputQubit_1 = new_inst_10.getResult(0);
  inputQubit_3 = new_inst_10.getResult(1);
   //           u3(0,0,pi) d;
  auto new_inst_11 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "rz", llvm::makeArrayRef({inputQubit_3}),
                      llvm::makeArrayRef({theta_PI}));
  inputQubit_3 = new_inst_11.getResult(0);
   //           cx c,d;
  auto new_inst_12 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type,qubit_type}),
                      "cx", llvm::makeArrayRef({inputQubit_2,inputQubit_3}),
                      llvm::None);
  inputQubit_2 = new_inst_12.getResult(0);
  inputQubit_3 = new_inst_12.getResult(1);
   //           u3(0,0,3*pi/4) d;
  auto new_inst_13 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "rz", llvm::makeArrayRef({inputQubit_3}),
                      llvm::makeArrayRef({theta_PI_3_4}));
  inputQubit_3 = new_inst_13.getResult(0);
  op.getResult(0).replaceAllUsesWith(inputQubit_0);
  op.getResult(1).replaceAllUsesWith(inputQubit_1);
  op.getResult(2).replaceAllUsesWith(inputQubit_2);
  op.getResult(3).replaceAllUsesWith(inputQubit_3);
}

 // {"c3x",R"#(gate c3x a,b,c,d
 // {
 //     h d;
 //     p(pi/8) a;
 //     p(pi/8) b;
 //     p(pi/8) c;
 //     p(pi/8) d;
 //     cx a, b;
 //     p(-pi/8) b;
 //     cx a, b;
 //     cx b, c;
 //     p(-pi/8) c;
 //     cx a, c;
 //     p(pi/8) c;
 //     cx b, c;
 //     p(-pi/8) c;
 //     cx a, c;
 //     cx c, d;
 //     p(-pi/8) d;
 //     cx b, d;
 //     p(pi/8) d;
 //     cx c, d;
 //     p(-pi/8) d;
 //     cx a, d;
 //     p(pi/8) d;
 //     cx c, d;
 //     p(-pi/8) d;
 //     cx b, d;
 //     p(pi/8) d;
 //     cx c, d;
 //     p(-pi/8) d;
 //     cx a, d;
 //     h d;
 // })#"},
void decompose_c3x_gate(mlir::quantum::ValueSemanticsInstOp &op,std::vector<mlir::quantum::ValueSemanticsInstOp> &deadOps){
  deadOps.emplace_back(op);
  mlir::OpBuilder rewriter(op);
  rewriter.setInsertionPointAfter(op);

  std::vector<mlir::Value> param;
  mlir::Value inputQubit_0 = op.getOperand(0);
  mlir::Value inputQubit_1 = op.getOperand(1);
  mlir::Value inputQubit_2 = op.getOperand(2);
  mlir::Value inputQubit_3 = op.getOperand(3);
  
  mlir::Value theta_PI_8 = rewriter.create<mlir::ConstantOp>(
                    op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), M_PI_4/2.0));
  mlir::Value theta_neg_PI_8 = rewriter.create<mlir::ConstantOp>(
                    op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), -1*M_PI_4/2.0));

  mlir::Type qubit_type = inputQubit_0.getType();
   //h d; 
  auto new_inst_1 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "h", llvm::makeArrayRef({inputQubit_3}),
                      llvm::None);
  inputQubit_3 = new_inst_1.getResult(0);
 //     p(pi/8) a;
  auto new_inst_2 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "rz", llvm::makeArrayRef({inputQubit_0}),
                      llvm::makeArrayRef({theta_PI_8}));
  inputQubit_0 = new_inst_2.getResult(0);
 //     p(pi/8) b;
  auto new_inst_3 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "rz", llvm::makeArrayRef({inputQubit_1}),
                      llvm::makeArrayRef({theta_PI_8}));
  inputQubit_1 = new_inst_3.getResult(0);
 //     p(pi/8) c;
  auto new_inst_4 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "rz", llvm::makeArrayRef({inputQubit_2}),
                      llvm::makeArrayRef({theta_PI_8}));
  inputQubit_2 = new_inst_4.getResult(0);
 //     p(pi/8) d;
  auto new_inst_5 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "rz", llvm::makeArrayRef({inputQubit_3}),
                      llvm::makeArrayRef({theta_PI_8}));
  inputQubit_3 = new_inst_5.getResult(0);
 //     cx a, b;
  auto new_inst_6 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type,qubit_type}),
                      "cx", llvm::makeArrayRef({inputQubit_0,inputQubit_1}),
                      llvm::None);
  inputQubit_0 = new_inst_6.getResult(0);
  inputQubit_1= new_inst_6.getResult(1);
 //     p(-pi/8) b;
  auto new_inst_7 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "rz", llvm::makeArrayRef({inputQubit_1}),
                      llvm::makeArrayRef({theta_neg_PI_8}));
  inputQubit_1 = new_inst_7.getResult(0);
 //     cx a, b;
  auto new_inst_8 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type,qubit_type}),
                      "cx", llvm::makeArrayRef({inputQubit_0,inputQubit_1}),
                      llvm::None);
  inputQubit_0 = new_inst_8.getResult(0);
  inputQubit_1= new_inst_8.getResult(1);
 //     cx b, c;
  auto new_inst_9 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type,qubit_type}),
                      "cx", llvm::makeArrayRef({inputQubit_1,inputQubit_2}),
                      llvm::None);
  inputQubit_1 = new_inst_9.getResult(0);
  inputQubit_2= new_inst_9.getResult(1);
 //     p(-pi/8) c;
  auto new_inst_10 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "rz", llvm::makeArrayRef({inputQubit_2}),
                      llvm::makeArrayRef({theta_neg_PI_8}));
  inputQubit_2 = new_inst_10.getResult(0);
 //     cx a, c;
  auto new_inst_11 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type,qubit_type}),
                      "cx", llvm::makeArrayRef({inputQubit_0,inputQubit_2}),
                      llvm::None);
  inputQubit_0 = new_inst_11.getResult(0);
  inputQubit_2= new_inst_11.getResult(1);
 //     p(pi/8) c;
  auto new_inst_12 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "rz", llvm::makeArrayRef({inputQubit_2}),
                      llvm::makeArrayRef({theta_PI_8}));
  inputQubit_2 = new_inst_12.getResult(0);
 //     cx b, c;
  auto new_inst_13 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type,qubit_type}),
                      "cx", llvm::makeArrayRef({inputQubit_1,inputQubit_2}),
                      llvm::None);
  inputQubit_1 = new_inst_13.getResult(0);
  inputQubit_2= new_inst_13.getResult(1);
 //     p(-pi/8) c;
  auto new_inst_14 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "rz", llvm::makeArrayRef({inputQubit_2}),
                      llvm::makeArrayRef({theta_neg_PI_8}));
  inputQubit_2 = new_inst_14.getResult(0);
 //     cx a, c;
  auto new_inst_15 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type,qubit_type}),
                      "cx", llvm::makeArrayRef({inputQubit_0,inputQubit_2}),
                      llvm::None);
  inputQubit_0 = new_inst_15.getResult(0);
  inputQubit_2= new_inst_15.getResult(1);
 //     cx c, d;
  auto new_inst_16 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type,qubit_type}),
                      "cx", llvm::makeArrayRef({inputQubit_2,inputQubit_3}),
                      llvm::None);
  inputQubit_2 = new_inst_16.getResult(0);
  inputQubit_3= new_inst_16.getResult(1);
 //     p(-pi/8) d;
  auto new_inst_17 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "rz", llvm::makeArrayRef({inputQubit_3}),
                      llvm::makeArrayRef({theta_neg_PI_8}));
  inputQubit_3 = new_inst_17.getResult(0);
 //     cx b, d;
  auto new_inst_18 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type,qubit_type}),
                      "cx", llvm::makeArrayRef({inputQubit_1,inputQubit_3}),
                      llvm::None);
  inputQubit_1 = new_inst_18.getResult(0);
  inputQubit_3= new_inst_18.getResult(1);
 //     p(pi/8) d;
  auto new_inst_19 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "rz", llvm::makeArrayRef({inputQubit_3}),
                      llvm::makeArrayRef({theta_PI_8}));
  inputQubit_3 = new_inst_19.getResult(0);
 //     cx c, d;
  auto new_inst_20 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type,qubit_type}),
                      "cx", llvm::makeArrayRef({inputQubit_2,inputQubit_3}),
                      llvm::None);
  inputQubit_2 = new_inst_20.getResult(0);
  inputQubit_3= new_inst_20.getResult(1);
 //     p(-pi/8) d;
  auto new_inst_21 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "rz", llvm::makeArrayRef({inputQubit_3}),
                      llvm::makeArrayRef({theta_neg_PI_8}));
  inputQubit_3 = new_inst_21.getResult(0);
 //     cx a, d;
  auto new_inst_22 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type,qubit_type}),
                      "cx", llvm::makeArrayRef({inputQubit_0,inputQubit_1}),
                      llvm::None);
  inputQubit_0 = new_inst_22.getResult(0);
  inputQubit_1= new_inst_22.getResult(1);
 //     p(pi/8) d;
  auto new_inst_23 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "rz", llvm::makeArrayRef({inputQubit_3}),
                      llvm::makeArrayRef({theta_PI_8}));
  inputQubit_3 = new_inst_23.getResult(0);
 //     cx c, d;
  auto new_inst_24 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type,qubit_type}),
                      "cx", llvm::makeArrayRef({inputQubit_2,inputQubit_3}),
                      llvm::None);
  inputQubit_2 = new_inst_24.getResult(0);
  inputQubit_3= new_inst_24.getResult(1);
 //     p(-pi/8) d;
  auto new_inst_25 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "rz", llvm::makeArrayRef({inputQubit_3}),
                      llvm::makeArrayRef({theta_neg_PI_8}));
  inputQubit_3 = new_inst_25.getResult(0);
 //     cx b, d;
  auto new_inst_26 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type,qubit_type}),
                      "cx", llvm::makeArrayRef({inputQubit_1,inputQubit_3}),
                      llvm::None);
  inputQubit_1 = new_inst_26.getResult(0);
  inputQubit_3= new_inst_26.getResult(1);
 //     p(pi/8) d;
  auto new_inst_27 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "rz", llvm::makeArrayRef({inputQubit_3}),
                      llvm::makeArrayRef({theta_PI_8}));
  inputQubit_3 = new_inst_27.getResult(0);
 //     cx c, d;
  auto new_inst_28 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type,qubit_type}),
                      "cx", llvm::makeArrayRef({inputQubit_2,inputQubit_3}),
                      llvm::None);
  inputQubit_2 = new_inst_28.getResult(0);
  inputQubit_3= new_inst_28.getResult(1);
 //     p(-pi/8) d;
  auto new_inst_29 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "rz", llvm::makeArrayRef({inputQubit_3}),
                      llvm::makeArrayRef({theta_neg_PI_8}));
  inputQubit_3 = new_inst_29.getResult(0);
 //     cx a, d;
  auto new_inst_30 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type,qubit_type}),
                      "cx", llvm::makeArrayRef({inputQubit_0,inputQubit_3}),
                      llvm::None);
  inputQubit_0 = new_inst_30.getResult(0);
  inputQubit_3= new_inst_30.getResult(1);
 //     h d;
  auto new_inst_31 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "h", llvm::makeArrayRef({inputQubit_3}),
                      llvm::None);
  inputQubit_3= new_inst_31.getResult(0);
  op.getResult(0).replaceAllUsesWith(inputQubit_0);
  op.getResult(1).replaceAllUsesWith(inputQubit_1);
  op.getResult(2).replaceAllUsesWith(inputQubit_2);
  op.getResult(3).replaceAllUsesWith(inputQubit_3);
}

 // {"c3sqrtx",R"#(gate c3sqrtx a,b,c,d
 //         {
 //             h d; cu1(pi/8) a,d; h d;
 //             cx a,b;
 //             h d; cu1(-pi/8) b,d; h d;
 //             cx a,b;
 //             h d; cu1(pi/8) b,d; h d;
 //             cx b,c;
 //             h d; cu1(-pi/8) c,d; h d;
 //             cx a,c;
 //             h d; cu1(pi/8) c,d; h d;
 //             cx b,c;
 //             h d; cu1(-pi/8) c,d; h d;
 //             cx a,c;
 //             h d; cu1(pi/8) c,d; h d;
 //         })#"},
void decompose_c3sqrtx_gate(mlir::quantum::ValueSemanticsInstOp &op,std::vector<mlir::quantum::ValueSemanticsInstOp> &deadOps){
  deadOps.emplace_back(op);
  mlir::OpBuilder rewriter(op);
  rewriter.setInsertionPointAfter(op);

  std::vector<mlir::Value> param;
  mlir::Value inputQubit_0 = op.getOperand(0);
  mlir::Value inputQubit_1 = op.getOperand(1);
  mlir::Value inputQubit_2 = op.getOperand(2);
  mlir::Value inputQubit_3 = op.getOperand(3);

  mlir::Value theta_PI_8 = rewriter.create<mlir::ConstantOp>(
                    op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), M_PI_4/2.0));
  mlir::Value theta_neg_PI_8 = rewriter.create<mlir::ConstantOp>(
                    op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), -1*M_PI_4/2.0));

  mlir::Type qubit_type = inputQubit_0.getType();
   //  h d; cu1(pi/8) a,d; h d;
   //h d; 
  auto new_inst_1 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "h", llvm::makeArrayRef({inputQubit_3}),
                      llvm::None);
  inputQubit_3 = new_inst_1.getResult(0);

   // cu1(pi/8) a,d;
  auto new_inst_2 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type,qubit_type}),
                      "cu1", llvm::makeArrayRef({inputQubit_0,inputQubit_3}),
                      llvm::makeArrayRef({theta_PI_8}));
  inputQubit_0 = new_inst_2.getResult(0);
  inputQubit_3 = new_inst_2.getResult(1);

   // h d;
  auto new_inst_3 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "h", llvm::makeArrayRef({inputQubit_3}),
                      llvm::None);
  inputQubit_3 = new_inst_3.getResult(0);

   //cx a,b;
  auto new_inst_4 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type,qubit_type}),
                      "cx", llvm::makeArrayRef({inputQubit_0,inputQubit_1}),
                      llvm::None);
  inputQubit_0 = new_inst_4.getResult(0);
  inputQubit_1 = new_inst_4.getResult(1);
  
   //  h d; cu1(-pi/8) b,d; h d;

   //h d; 
  auto new_inst_5 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "h", llvm::makeArrayRef({inputQubit_3}),
                      llvm::None);
  inputQubit_3 = new_inst_5.getResult(0);
   // cu1(-pi/8) b,d;   
  auto new_inst_6 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type,qubit_type}),
                      "cu1", llvm::makeArrayRef({inputQubit_1,inputQubit_3}),
                      llvm::makeArrayRef({theta_neg_PI_8}));
  inputQubit_1 = new_inst_6.getResult(0);
  inputQubit_3 = new_inst_6.getResult(1);
   //h d;
  auto new_inst_7 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "h", llvm::makeArrayRef({inputQubit_3}),
                      llvm::None);
  inputQubit_3 = new_inst_7.getResult(0);
   // cx a,b;
  auto new_inst_8 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type,qubit_type}),
                      "cx", llvm::makeArrayRef({inputQubit_0,inputQubit_1}),
                      llvm::None);
  inputQubit_0 = new_inst_8.getResult(0);
  inputQubit_1 = new_inst_8.getResult(1);

   // h d; cu1(pi/8) b,d; h d;

   // h d;
  auto new_inst_9 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "h", llvm::makeArrayRef({inputQubit_3}),
                      llvm::None);
  inputQubit_3 = new_inst_9.getResult(0);                  
   //  cu1(pi/8) b,d;
  auto new_inst_10 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type,qubit_type}),
                      "cu1", llvm::makeArrayRef({inputQubit_1,inputQubit_3}),
                      llvm::makeArrayRef({theta_PI_8}));
  inputQubit_0 = new_inst_10.getResult(0);
  inputQubit_3 = new_inst_10.getResult(1);
   // h d;
  auto new_inst_11 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "h", llvm::makeArrayRef({inputQubit_3}),
                      llvm::None);
  inputQubit_3 = new_inst_11.getResult(0);
   //cx b,c;
  auto new_inst_12 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type,qubit_type}),
                      "cx", llvm::makeArrayRef({inputQubit_1,inputQubit_2}),
                      llvm::None);
  inputQubit_2 = new_inst_12.getResult(0);
  inputQubit_2 = new_inst_12.getResult(1);
   //  h d; cu1(-pi/8) c,d; h d;

   // h d; 
  auto new_inst_13 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "h", llvm::makeArrayRef({inputQubit_3}),
                      llvm::None);
  inputQubit_3 = new_inst_13.getResult(0);
   // cu1(-pi/8) c,d;   
  auto new_inst_14 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type,qubit_type}),
                      "cu1", llvm::makeArrayRef({inputQubit_2,inputQubit_3}),
                      llvm::makeArrayRef({theta_neg_PI_8}));
  inputQubit_2 = new_inst_14.getResult(0);
  inputQubit_3 = new_inst_14.getResult(1);
     //  h d;
  auto new_inst_15 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "h", llvm::makeArrayRef({inputQubit_3}),
                      llvm::None);
  inputQubit_3 = new_inst_15.getResult(0);
   //cx a,c;
  auto new_inst_16 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type,qubit_type}),
                      "cx", llvm::makeArrayRef({inputQubit_0,inputQubit_2}),
                      llvm::None);
  inputQubit_0 = new_inst_16.getResult(0);
  inputQubit_2 = new_inst_16.getResult(1);
   //             h d; cu1(pi/8) c,d; h d;

   //h d; 
  auto new_inst_17 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "h", llvm::makeArrayRef({inputQubit_3}),
                      llvm::None);
  inputQubit_3 = new_inst_17.getResult(0);
   // cu1(pi/8) c,d;   
  auto new_inst_18 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type,qubit_type}),
                      "cu1", llvm::makeArrayRef({inputQubit_2,inputQubit_3}),
                      llvm::makeArrayRef({theta_PI_8}));
  inputQubit_2 = new_inst_18.getResult(0);
  inputQubit_3 = new_inst_18.getResult(1);
   // h d;
  auto new_inst_27 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "h", llvm::makeArrayRef({inputQubit_3}),
                      llvm::None);
  inputQubit_3 = new_inst_27.getResult(0);
   // cx b,c;
  auto new_inst_19 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type,qubit_type}),
                      "cx", llvm::makeArrayRef({inputQubit_1,inputQubit_2}),
                      llvm::None);
  inputQubit_1 = new_inst_19.getResult(0);
  inputQubit_2 = new_inst_19.getResult(1);
 //             h d; cu1(-pi/8) c,d; h d;

   // h d;
  auto new_inst_20 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "h", llvm::makeArrayRef({inputQubit_3}),
                      llvm::None);
  inputQubit_3 = new_inst_20.getResult(0);                  
   // cu1(-pi/8) c,d; 
  auto new_inst_21 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type,qubit_type}),
                      "cu1", llvm::makeArrayRef({inputQubit_2,inputQubit_3}),
                      llvm::makeArrayRef({theta_neg_PI_8}));
  inputQubit_2 = new_inst_21.getResult(0);
  inputQubit_3 = new_inst_21.getResult(1);
   // h d;
  auto new_inst_26 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "h", llvm::makeArrayRef({inputQubit_3}),
                      llvm::None);
  inputQubit_3 = new_inst_26.getResult(0); 
   //  cx a,c;
  auto new_inst_22 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type,qubit_type}),
                      "cx", llvm::makeArrayRef({inputQubit_0,inputQubit_2}),
                      llvm::None);
  inputQubit_0 = new_inst_22.getResult(0);
  inputQubit_2 = new_inst_22.getResult(1);
 //             h d; cu1(pi/8) c,d; h d;
   // h d;
  auto new_inst_23 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "h", llvm::makeArrayRef({inputQubit_3}),
                      llvm::None);
  inputQubit_3 = new_inst_23.getResult(0);
   //  cu1(pi/8) c,d; 
  auto new_inst_24 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type,qubit_type}),
                      "cu1", llvm::makeArrayRef({inputQubit_2,inputQubit_3}),
                      llvm::makeArrayRef({theta_PI_8}));
  inputQubit_2 = new_inst_24.getResult(0);
  inputQubit_3 = new_inst_24.getResult(1);
   //  h d; 
  auto new_inst_25 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "h", llvm::makeArrayRef({inputQubit_3}),
                      llvm::None);
  inputQubit_3 = new_inst_25.getResult(0);

  op.getResult(0).replaceAllUsesWith(inputQubit_0);
  op.getResult(1).replaceAllUsesWith(inputQubit_1);
  op.getResult(2).replaceAllUsesWith(inputQubit_2);
  op.getResult(3).replaceAllUsesWith(inputQubit_3);
  decompose_cu1_gate(new_inst_2,deadOps);
  decompose_cu1_gate(new_inst_6,deadOps);
  decompose_cu1_gate(new_inst_10,deadOps);
  decompose_cu1_gate(new_inst_14,deadOps);
  decompose_cu1_gate(new_inst_18,deadOps);
  decompose_cu1_gate(new_inst_21,deadOps);
  decompose_cu1_gate(new_inst_24,deadOps);

}

 // {"c4x",R"#(gate c4x a,b,c,d,e { h e; cu1(pi/2) d,e; h e; rc3x a,b,c,d; h e; cu1(-pi/2) d,e; h e; rc3x a,b,c,d; c3sqrtx a,b,c,e;})#"},
void decompose_c4x_gate(mlir::quantum::ValueSemanticsInstOp &op,std::vector<mlir::quantum::ValueSemanticsInstOp> &deadOps){
  deadOps.emplace_back(op);
  mlir::OpBuilder rewriter(op);
  rewriter.setInsertionPointAfter(op);

  std::vector<mlir::Value> param;
  mlir::Value inputQubit_0 = op.getOperand(0);
  mlir::Value inputQubit_1 = op.getOperand(1);
  mlir::Value inputQubit_2 = op.getOperand(2);
  mlir::Value inputQubit_3 = op.getOperand(3);
  mlir::Value inputQubit_4 = op.getOperand(4);

  mlir::Value theta_zero = rewriter.create<mlir::ConstantOp>(
                    op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), 0));
  mlir::Value theta_PI_2 = rewriter.create<mlir::ConstantOp>(
                    op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), M_PI_2));
  mlir::Value theta_neg_PI_2 = rewriter.create<mlir::ConstantOp>(
                    op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), -1*M_PI_2));

  mlir::Type qubit_type = inputQubit_0.getType();

   //h e; 

  auto new_inst_1 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "h", llvm::makeArrayRef({inputQubit_4}),
                      llvm::None);
  inputQubit_4 = new_inst_1.getResult(0);

   //cu1(pi/2) d,e;  
  param.emplace_back(theta_PI_2);
  
  auto new_inst_2 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type,qubit_type}),
                      "cu1", llvm::makeArrayRef({inputQubit_3,inputQubit_4}),
                      llvm::makeArrayRef(param));
  param.clear();
  inputQubit_3 = new_inst_2.getResult(0);
  inputQubit_4 = new_inst_2.getResult(1);

   // h e; 
  auto new_inst_3 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "h", llvm::makeArrayRef({inputQubit_4}),
                      llvm::None);
  inputQubit_4 = new_inst_3.getResult(0);

   //rc3x a,b,c,d; 

  auto new_inst_4 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type,qubit_type,qubit_type,qubit_type}),
                      "rc3x", llvm::makeArrayRef({inputQubit_0,inputQubit_1,inputQubit_2,inputQubit_3}),
                      llvm::None);
  inputQubit_0 = new_inst_4.getResult(0);
  inputQubit_1 = new_inst_4.getResult(1);
  inputQubit_2 = new_inst_4.getResult(2);
  inputQubit_3 = new_inst_4.getResult(3);
  
   // h e; 
  auto new_inst_5 =  rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "h", llvm::makeArrayRef({inputQubit_4}),
                      llvm::None);
  inputQubit_4 = new_inst_5.getResult(0);
   // cu1(-pi/2) d,e; 

  param.emplace_back(theta_neg_PI_2);

  auto new_inst_6 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type,qubit_type}),
                      "cu1", llvm::makeArrayRef({inputQubit_3,inputQubit_4}),
                      llvm::makeArrayRef(param));
  inputQubit_3 = new_inst_6.getResult(0);
  inputQubit_4 = new_inst_6.getResult(1);
  param.clear();
   //h e;  
  auto new_inst_7 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "h", llvm::makeArrayRef({inputQubit_4}),
                      llvm::None);
  inputQubit_4 = new_inst_7.getResult(0);
   // rc3x a,b,c,d; 
  auto new_inst_8 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type,qubit_type,qubit_type,qubit_type}),
                      "rc3x", llvm::makeArrayRef({inputQubit_0,inputQubit_1,inputQubit_2,inputQubit_3}),
                      llvm::makeArrayRef(param));
  inputQubit_0 = new_inst_8.getResult(0);
  inputQubit_1 = new_inst_8.getResult(1);
  inputQubit_2 = new_inst_8.getResult(2);
  inputQubit_3 = new_inst_8.getResult(3);
  
   //  c3sqrtx a,b,c,e;
  auto new_inst_9 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type,qubit_type,qubit_type,qubit_type}),
                      "c3sqrtx", llvm::makeArrayRef({inputQubit_0,inputQubit_1,inputQubit_2,inputQubit_4}),
                      llvm::makeArrayRef(param));
  inputQubit_0 = new_inst_8.getResult(0);
  inputQubit_1 = new_inst_8.getResult(1);
  inputQubit_2 = new_inst_8.getResult(2);
  inputQubit_4 = new_inst_8.getResult(3);

  op.getResult(0).replaceAllUsesWith(inputQubit_0);
  op.getResult(1).replaceAllUsesWith(inputQubit_1);
  op.getResult(2).replaceAllUsesWith(inputQubit_2);
  op.getResult(3).replaceAllUsesWith(inputQubit_3);
  op.getResult(4).replaceAllUsesWith(inputQubit_4);

  decompose_cu1_gate(new_inst_2,deadOps);
  decompose_rc3x_gate(new_inst_4,deadOps);
  decompose_cu1_gate(new_inst_6,deadOps);
  decompose_rc3x_gate(new_inst_8,deadOps);
  decompose_c3sqrtx_gate(new_inst_9,deadOps);
}

 // {"cpx",R"#(gate cpx(theta) a,b {rz(theta/2) a; cx a,b; u3(0,0,-theta/2) b; cx a,b; u3(0,0,theta/2) b; cx b,a;})#"},
void decompose_cpx_gate(mlir::quantum::ValueSemanticsInstOp &op,std::vector<mlir::quantum::ValueSemanticsInstOp> &deadOps){
  deadOps.emplace_back(op);
  mlir::OpBuilder rewriter(op);
  rewriter.setInsertionPointAfter(op);

  std::vector<mlir::Value> param;
  mlir::Value inputQubit_0 = op.getOperand(0);
  mlir::Value inputQubit_1 = op.getOperand(1);

  mlir::Value theta = op.getOperand(2);
  auto theta_d = qllvm::OP::tryGetConstAngle(theta);

  mlir::Value theta_zero = rewriter.create<mlir::ConstantOp>(
                    op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), 0));
  mlir::Value theta_h = rewriter.create<mlir::ConstantOp>(
                    op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), theta_d/2.0));
  mlir::Value theta_h_neg = rewriter.create<mlir::ConstantOp>(
                    op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), -1*theta_d/2.0));
  mlir::Type qubit_type = inputQubit_0.getType();
  
   // rz(theta/2) a;
  auto new_inst_1 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "rz", llvm::makeArrayRef({inputQubit_0}),
                      llvm::makeArrayRef({theta_h}));
  inputQubit_0 = new_inst_1.getResult(0);
   //  cx a,b; 
  auto new_inst_2 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type,qubit_type}),
                      "cx", llvm::makeArrayRef({inputQubit_0,inputQubit_1}),
                      llvm::None);
  inputQubit_0 = new_inst_2.getResult(0); 
  inputQubit_1 = new_inst_2.getResult(1); 
   // u3(0,0,-theta/2) b; 
  param.emplace_back(theta_zero);
  param.emplace_back(theta_zero);
  param.emplace_back(theta_h_neg);
  auto new_inst_3 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "u3", llvm::makeArrayRef({inputQubit_1}),
                      llvm::makeArrayRef(param));
  param.clear();
  inputQubit_1 = new_inst_3.getResult(0);
   //  cx a,b; 
  auto new_inst_4 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type,qubit_type}),
                      "cx", llvm::makeArrayRef({inputQubit_0,inputQubit_1}),
                      llvm::None);
  inputQubit_0 = new_inst_4.getResult(0); 
  inputQubit_1 = new_inst_4.getResult(1); 
   // u3(0,0,theta/2) b;  
  param.emplace_back(theta_zero);
  param.emplace_back(theta_zero);
  param.emplace_back(theta_h);
  auto new_inst_5 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "u3", llvm::makeArrayRef({inputQubit_1}),
                      llvm::makeArrayRef(param));
  param.clear();
  inputQubit_1 = new_inst_5.getResult(0);
   // cx b,a;
  auto new_inst_6 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type,qubit_type}),
                      "cx", llvm::makeArrayRef({inputQubit_1,inputQubit_0}),
                      llvm::None);
  inputQubit_0 = new_inst_6.getResult(0); 
  inputQubit_1 = new_inst_6.getResult(1);
  op.getResult(0).replaceAllUsesWith(inputQubit_0);
  op.getResult(1).replaceAllUsesWith(inputQubit_1);

}

 // {"cxz",R"#(gate cxz(theta) a,b {cx a,b; rz(theta) b;})#"},
void decompose_cxz_gate(mlir::quantum::ValueSemanticsInstOp &op,std::vector<mlir::quantum::ValueSemanticsInstOp> &deadOps){
  deadOps.emplace_back(op);
  mlir::OpBuilder rewriter(op);
  rewriter.setInsertionPointAfter(op);

  std::vector<mlir::Value> param;
  mlir::Value inputQubit_0 = op.getOperand(0);
  mlir::Value inputQubit_1 = op.getOperand(1);
  mlir::Value theta = op.getOperand(2);


  mlir::Type qubit_type = inputQubit_0.getType();

  auto new_inst_1 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type,qubit_type}),
                      "cx", llvm::makeArrayRef({inputQubit_0,inputQubit_1}),
                      llvm::None);
  inputQubit_0 =  new_inst_1.getResult(0);         
  inputQubit_1 =  new_inst_1.getResult(1);

  auto new_inst_2 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "rz", llvm::makeArrayRef({inputQubit_1}),
                      llvm::makeArrayRef({theta}));

  op.getResult(0).replaceAllUsesWith(inputQubit_0);
  op.getResult(1).replaceAllUsesWith(new_inst_2.getResult(0));
}

void decompose_mcx_gate(mlir::quantum::ValueSemanticsInstOp &op,std::vector<mlir::quantum::ValueSemanticsInstOp> &deadOps){
  
  size_t num_qubit = op.getNumResults();
  size_t controlled_qubit = num_qubit - 1;

  if(num_qubit == 1){
    deadOps.emplace_back(op);
    mlir::OpBuilder rewriter(op);
    rewriter.setInsertionPointAfter(op);
    mlir::Value inputQubit_0 = op.getOperand(0);
    mlir::Type qubit_type = inputQubit_0.getType();

    auto new_inst_1 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                        op.getLoc(), llvm::makeArrayRef({qubit_type}),
                        "x", llvm::makeArrayRef({inputQubit_0}),
                        llvm::None);
    inputQubit_0 =  new_inst_1.getResult(0);
    op.getResult(0).replaceAllUsesWith(inputQubit_0);
  }else if(num_qubit == 2){  //cx
    deadOps.emplace_back(op);
    mlir::OpBuilder rewriter(op);
    rewriter.setInsertionPointAfter(op);
    mlir::Value inputQubit_0 = op.getOperand(0);
    mlir::Value inputQubit_1 = op.getOperand(1);
    mlir::Type qubit_type = inputQubit_0.getType();

    auto new_inst_1 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                        op.getLoc(), llvm::makeArrayRef({qubit_type,qubit_type}),
                        "cx", llvm::makeArrayRef({inputQubit_0,inputQubit_1}),
                        llvm::None);
    inputQubit_0 =  new_inst_1.getResult(0);         
    inputQubit_1 =  new_inst_1.getResult(1);
    op.getResult(0).replaceAllUsesWith(inputQubit_0);
    op.getResult(1).replaceAllUsesWith(inputQubit_1);
  }else if(num_qubit == 3){
    decompose_ccx_gate(op,deadOps);
  }else if(num_qubit == 4){
    decompose_c3x_gate(op,deadOps);
  }else{
    deadOps.emplace_back(op);
    
    mlir::OpBuilder rewriter(op);
    rewriter.setInsertionPointAfter(op);
    mlir::Value target_qubit = op.getOperand(controlled_qubit);
    mlir::Value input_qubit = op.getOperand(0);
    mlir::Type qubit_type = input_qubit.getType();
     // int last_control = controls.back();
    size_t last_c_num = controlled_qubit - 1;
    auto last_control = op.getOperand(last_c_num);

     //  step 1: apply CRX(π/2)
     // gate_list.push_back({"CRX", {last_control, target}, M_PI / 2.0});
    mlir::Value M_PI_2_v = rewriter.create<mlir::ConstantOp>(
                    op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), M_PI_2));
    mlir::Value neg_M_PI_2_v = rewriter.create<mlir::ConstantOp>(
                    op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), -1*M_PI_2));

    auto new_inst_1 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                        op.getLoc(), llvm::makeArrayRef({qubit_type,qubit_type}),
                        "crx", llvm::makeArrayRef({last_control,target_qubit}),
                        llvm::makeArrayRef({M_PI_2_v}));
    last_control = new_inst_1.getResult(0);
    target_qubit = new_inst_1.getResult(1);
     // step 2: recursively call C^{n-1}-X
     //  auto sub_decomposition1 = decompose_mcx(other_controls, last_control);
    std::vector<mlir::Value> other_controls;
    std::vector<mlir::Type> qubit_type_v;
    std::unordered_map<size_t, mlir::Value> control_v;

    for(size_t i = 0; i <= controlled_qubit-2;i++){
      other_controls.emplace_back(op.getOperand(i));
      qubit_type_v.emplace_back(qubit_type);
      control_v[i] = op.getOperand(i);
    }

    other_controls.emplace_back(last_control);
    qubit_type_v.emplace_back(qubit_type);

    auto new_inst_2 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                        op.getLoc(), llvm::makeArrayRef(qubit_type_v),
                        "mcx", llvm::makeArrayRef(other_controls),
                        llvm::None);
    last_control = new_inst_2.getResult(last_c_num);
    for(size_t i = 0; i <= controlled_qubit-2;i++){
      control_v[i] = new_inst_2.getResult(i);
    }

     // step 3: apply CRX(-π/2)
     // gate_list.push_back({"CRX", {last_control, target}, -M_PI / 2.0});
    auto new_inst_3 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                        op.getLoc(), llvm::makeArrayRef({qubit_type,qubit_type}),
                        "crx", llvm::makeArrayRef({last_control,target_qubit}),
                        llvm::makeArrayRef({neg_M_PI_2_v}));
    last_control = new_inst_3.getResult(0);
    target_qubit = new_inst_3.getResult(1);

     // step 4: recursively call C^{n-1}-X
     // auto sub_decomposition2 = decompose_mcx(other_controls, last_control);
    other_controls.clear();
    for(int i = 0; i <= controlled_qubit-2;i++){
      other_controls.emplace_back(control_v[i]);
    }
    other_controls.emplace_back(last_control);
    auto new_inst_4 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                        op.getLoc(), llvm::makeArrayRef(qubit_type_v),
                        "mcx", llvm::makeArrayRef(other_controls),
                        llvm::None);
    for(int i = 0; i <= controlled_qubit-2;i++){
      control_v[i] = new_inst_4.getResult(i);
    }
    last_control = new_inst_4.getResult(controlled_qubit-1);

     //  step 5: another recursively call C^{n-1}-X
     // auto sub_decomposition3 = decompose_mcx(other_controls, target);
    other_controls.clear();
    for(int i = 0; i < controlled_qubit-1;i++){
      other_controls.emplace_back(control_v[i]);
    }
    other_controls.emplace_back(target_qubit);
    auto new_inst_5 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                        op.getLoc(), llvm::makeArrayRef(qubit_type_v),
                        "mcx", llvm::makeArrayRef(other_controls),
                        llvm::None);
    for(int i = 0; i <= controlled_qubit-2;i++){
      control_v[i] = new_inst_5.getResult(i);
    }
    target_qubit = new_inst_5.getResult(controlled_qubit-1);

    for(int i = 0; i <= controlled_qubit-2;i++){
      op.getResult(i).replaceAllUsesWith(control_v[i]);
    }

    op.getResult(controlled_qubit).replaceAllUsesWith(target_qubit);
    op.getResult(controlled_qubit-1).replaceAllUsesWith(last_control);

    decompose_crx_gate(new_inst_1,deadOps);
    decompose_mcx_gate(new_inst_2,deadOps);
    decompose_crx_gate(new_inst_3,deadOps);
    decompose_mcx_gate(new_inst_4,deadOps);
    decompose_mcx_gate(new_inst_5,deadOps);
  }
}

void decompose_mcp_gate(mlir::quantum::ValueSemanticsInstOp &op,std::vector<mlir::quantum::ValueSemanticsInstOp> &deadOps){
  
  size_t num_qubit = op.getNumResults();
  size_t controlled_qubit = num_qubit - 1;

  if(num_qubit == 1){  //p
    deadOps.emplace_back(op);
    mlir::OpBuilder rewriter(op);
    rewriter.setInsertionPointAfter(op);
    mlir::Value inputQubit_0 = op.getOperand(0);
    mlir::Value theta = op.getOperand(1);
    mlir::Type qubit_type = inputQubit_0.getType();

    auto new_inst_1 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                        op.getLoc(), llvm::makeArrayRef({qubit_type}),
                        "p", llvm::makeArrayRef({inputQubit_0}),
                        llvm::makeArrayRef({theta}));
    inputQubit_0 =  new_inst_1.getResult(0);
    op.getResult(0).replaceAllUsesWith(inputQubit_0);
  }else if(num_qubit == 2){  //cp
    deadOps.emplace_back(op);
    mlir::OpBuilder rewriter(op);
    rewriter.setInsertionPointAfter(op);
    mlir::Value inputQubit_0 = op.getOperand(0);
    mlir::Value inputQubit_1 = op.getOperand(1);
    mlir::Value theta = op.getOperand(2);
    mlir::Type qubit_type = inputQubit_0.getType();

    auto new_inst_1 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                        op.getLoc(), llvm::makeArrayRef({qubit_type,qubit_type}),
                        "cp", llvm::makeArrayRef({inputQubit_0,inputQubit_1}),
                        llvm::makeArrayRef({theta}));
    inputQubit_0 =  new_inst_1.getResult(0);         
    inputQubit_1 =  new_inst_1.getResult(1);
    op.getResult(0).replaceAllUsesWith(inputQubit_0);
    op.getResult(1).replaceAllUsesWith(inputQubit_1);
  }else{
    deadOps.emplace_back(op);
    
    mlir::OpBuilder rewriter(op);
    rewriter.setInsertionPointAfter(op);
    mlir::Value target_qubit = op.getOperand(controlled_qubit);
    mlir::Value input_qubit = op.getOperand(0);
    mlir::Value theta = op.getOperand(num_qubit);

    auto theta_d = qllvm::OP::tryGetConstAngle(theta);

    mlir::Type qubit_type = input_qubit.getType();
     // int last_control = controls.back();
    size_t last_c_num = controlled_qubit - 1;
    auto last_control = op.getOperand(last_c_num);

    mlir::Value angle_2_v = rewriter.create<mlir::ConstantOp>(
                    op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), theta_d/2.0));
    mlir::Value neg_angle_2_v = rewriter.create<mlir::ConstantOp>(
                    op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), -1*theta_d/2.0));

     //  step 1: apply C-P(λ/2)
     // gate_list.extend(decompose_cp(angle / 2, last_control, target))
    auto new_inst_1 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                        op.getLoc(), llvm::makeArrayRef({qubit_type,qubit_type}),
                        "cp", llvm::makeArrayRef({last_control,target_qubit}),
                        llvm::makeArrayRef({angle_2_v}));
    last_control = new_inst_1.getResult(0);
    target_qubit = new_inst_1.getResult(1);

     // step 2: apply C^{n-1}-X
     // gate_list.extend(decompose_mcx_to_cnot(other_controls, last_control))
    std::vector<mlir::Value> other_controls;
    std::vector<mlir::Type> qubit_type_v;
    std::unordered_map<size_t, mlir::Value> control_v;

    for(size_t i = 0; i <= controlled_qubit-2;i++){
      other_controls.emplace_back(op.getOperand(i));
      qubit_type_v.emplace_back(qubit_type);
      control_v[i] = op.getOperand(i);
    }

    other_controls.emplace_back(last_control);
    qubit_type_v.emplace_back(qubit_type);

    auto new_inst_2 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                        op.getLoc(), llvm::makeArrayRef(qubit_type_v),
                        "mcx", llvm::makeArrayRef(other_controls),
                        llvm::None);
    last_control = new_inst_2.getResult(last_c_num);
    for(size_t i = 0; i <= controlled_qubit-2;i++){
      control_v[i] = new_inst_2.getResult(i);
    }

     // step 3: apply C-P(-λ/2)
     // gate_list.extend(decompose_cp(-angle / 2, last_control, target))
    auto new_inst_3 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                        op.getLoc(), llvm::makeArrayRef({qubit_type,qubit_type}),
                        "cp", llvm::makeArrayRef({last_control,target_qubit}),
                        llvm::makeArrayRef({neg_angle_2_v}));
    last_control = new_inst_3.getResult(0);
    target_qubit = new_inst_3.getResult(1);

     // step 4: again apply C^{n-1}-X
     // gate_list.extend(decompose_mcx_to_cnot(other_controls, last_control))
    other_controls.clear();
    for(int i = 0; i <= controlled_qubit-2;i++){
      other_controls.emplace_back(control_v[i]);
    }
    other_controls.emplace_back(last_control);
    auto new_inst_4 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                        op.getLoc(), llvm::makeArrayRef(qubit_type_v),
                        "mcx", llvm::makeArrayRef(other_controls),
                        llvm::None);
    for(int i = 0; i <= controlled_qubit-2;i++){
      control_v[i] = new_inst_4.getResult(i);
    }
    last_control = new_inst_4.getResult(controlled_qubit-1);

     // step 5: recursively call C^{n-1}-P(λ/2)
     // gate_list.extend(decompose_mcp(angle / 2, other_controls, target))
    other_controls.clear();
    for(int i = 0; i < controlled_qubit-1;i++){
      other_controls.emplace_back(control_v[i]);
    }
    other_controls.emplace_back(target_qubit);
    auto new_inst_5 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                        op.getLoc(), llvm::makeArrayRef(qubit_type_v),
                        "mcp", llvm::makeArrayRef(other_controls),
                        llvm::makeArrayRef({angle_2_v}));
    for(int i = 0; i <= controlled_qubit-2;i++){
      control_v[i] = new_inst_5.getResult(i);
    }
    target_qubit = new_inst_5.getResult(controlled_qubit-1);

    for(int i = 0; i <= controlled_qubit-2;i++){
      op.getResult(i).replaceAllUsesWith(control_v[i]);
    }

    op.getResult(controlled_qubit).replaceAllUsesWith(target_qubit);
    op.getResult(controlled_qubit-1).replaceAllUsesWith(last_control);

    decompose_mcx_gate(new_inst_2,deadOps);
    decompose_mcx_gate(new_inst_4,deadOps);
    decompose_mcp_gate(new_inst_5,deadOps);
  }
}

void decompose_mcrx_gate(mlir::quantum::ValueSemanticsInstOp &op,std::vector<mlir::quantum::ValueSemanticsInstOp> &deadOps){
  
  size_t num_qubit = op.getNumResults();
  size_t controlled_qubit = num_qubit - 1;

  if(num_qubit == 1){
    deadOps.emplace_back(op);
    mlir::OpBuilder rewriter(op);
    rewriter.setInsertionPointAfter(op);
    mlir::Value inputQubit_0 = op.getOperand(0);
    mlir::Value theta = op.getOperand(1);
    mlir::Type qubit_type = inputQubit_0.getType();

    auto new_inst_1 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                        op.getLoc(), llvm::makeArrayRef({qubit_type}),
                        "rx", llvm::makeArrayRef({inputQubit_0}),
                        llvm::makeArrayRef({theta}));
    inputQubit_0 =  new_inst_1.getResult(0);
    op.getResult(0).replaceAllUsesWith(inputQubit_0);
  }else if(num_qubit == 2){  //crx
    decompose_crx_gate(op,deadOps);
  }else{
    deadOps.emplace_back(op);
    
    mlir::OpBuilder rewriter(op);
    rewriter.setInsertionPointAfter(op);
    mlir::Value target_qubit = op.getOperand(controlled_qubit);
    mlir::Value input_qubit = op.getOperand(0);
    mlir::Value theta = op.getOperand(num_qubit);

    auto theta_d = qllvm::OP::tryGetConstAngle(theta);

    mlir::Type qubit_type = input_qubit.getType();
     // int last_control = controls.back();
    size_t last_c_num = controlled_qubit - 1;
    auto last_control = op.getOperand(last_c_num);

    mlir::Value angle_2_v = rewriter.create<mlir::ConstantOp>(
                    op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), theta_d/2.0));
    mlir::Value neg_angle_2_v = rewriter.create<mlir::ConstantOp>(
                    op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), -1*theta_d/2.0));

     // step 1: apply C-RX(θ/2)
     // gate_list.extend(decompose_crx(angle / 2, last_control, target))
    auto new_inst_1 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                        op.getLoc(), llvm::makeArrayRef({qubit_type,qubit_type}),
                        "crx", llvm::makeArrayRef({last_control,target_qubit}),
                        llvm::makeArrayRef({angle_2_v}));
    last_control = new_inst_1.getResult(0);
    target_qubit = new_inst_1.getResult(1);
     // step 2: apply C^{n-1}-X
     // gate_list.extend(decompose_mcx_to_cnot(other_controls, last_control))
    std::vector<mlir::Value> other_controls;
    std::vector<mlir::Type> qubit_type_v;
    std::unordered_map<size_t, mlir::Value> control_v;

    for(size_t i = 0; i <= controlled_qubit-2;i++){
      other_controls.emplace_back(op.getOperand(i));
      qubit_type_v.emplace_back(qubit_type);
      control_v[i] = op.getOperand(i);
    }

    other_controls.emplace_back(last_control);
    qubit_type_v.emplace_back(qubit_type);

    auto new_inst_2 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                        op.getLoc(), llvm::makeArrayRef(qubit_type_v),
                        "mcx", llvm::makeArrayRef(other_controls),
                        llvm::None);
    last_control = new_inst_2.getResult(last_c_num);
    for(size_t i = 0; i <= controlled_qubit-2;i++){
      control_v[i] = new_inst_2.getResult(i);
    }
     // step 3: apply C-RX(-θ/2)
     // gate_list.extend(decompose_crx(-angle / 2, last_control, target))
    auto new_inst_3 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                        op.getLoc(), llvm::makeArrayRef({qubit_type,qubit_type}),
                        "crx", llvm::makeArrayRef({last_control,target_qubit}),
                        llvm::makeArrayRef({neg_angle_2_v}));
    last_control = new_inst_3.getResult(0);
    target_qubit = new_inst_3.getResult(1);
     // step 4: again apply C^{n-1}-X
     // gate_list.extend(decompose_mcx_to_cnot(other_controls, last_control))
    other_controls.clear();
    for(int i = 0; i <= controlled_qubit-2;i++){
      other_controls.emplace_back(control_v[i]);
    }
    other_controls.emplace_back(last_control);
    auto new_inst_4 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                        op.getLoc(), llvm::makeArrayRef(qubit_type_v),
                        "mcx", llvm::makeArrayRef(other_controls),
                        llvm::None);
    for(int i = 0; i <= controlled_qubit-2;i++){
      control_v[i] = new_inst_4.getResult(i);
    }
    last_control = new_inst_4.getResult(controlled_qubit-1);
     // Step 5: recursively call C^{n-1}-RX(θ/2)
     // gate_list.extend(decompose_mcrx(angle / 2, other_controls, target))
    other_controls.clear();
    for(int i = 0; i < controlled_qubit-1;i++){
      other_controls.emplace_back(control_v[i]);
    }
    other_controls.emplace_back(target_qubit);
    auto new_inst_5 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                        op.getLoc(), llvm::makeArrayRef(qubit_type_v),
                        "mcrx", llvm::makeArrayRef(other_controls),
                        llvm::makeArrayRef({angle_2_v}));
    for(int i = 0; i <= controlled_qubit-2;i++){
      control_v[i] = new_inst_5.getResult(i);
    }
    target_qubit = new_inst_5.getResult(controlled_qubit-1);

    for(int i = 0; i <= controlled_qubit-2;i++){
      op.getResult(i).replaceAllUsesWith(control_v[i]);
    }

    op.getResult(controlled_qubit).replaceAllUsesWith(target_qubit);
    op.getResult(controlled_qubit-1).replaceAllUsesWith(last_control);

    decompose_crx_gate(new_inst_1,deadOps);
    decompose_mcx_gate(new_inst_2,deadOps);
    decompose_crx_gate(new_inst_3,deadOps);
    decompose_mcx_gate(new_inst_4,deadOps);
    decompose_mcrx_gate(new_inst_5,deadOps);
  }
}

void decompose_mcry_gate(mlir::quantum::ValueSemanticsInstOp &op,std::vector<mlir::quantum::ValueSemanticsInstOp> &deadOps){
  
  size_t num_qubit = op.getNumResults();
  size_t controlled_qubit = num_qubit - 1;

  if(num_qubit == 1){  //ry
    deadOps.emplace_back(op);
    mlir::OpBuilder rewriter(op);
    rewriter.setInsertionPointAfter(op);
    mlir::Value inputQubit_0 = op.getOperand(0);
    mlir::Value theta = op.getOperand(1);
    mlir::Type qubit_type = inputQubit_0.getType();

    auto new_inst_1 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                        op.getLoc(), llvm::makeArrayRef({qubit_type}),
                        "ry", llvm::makeArrayRef({inputQubit_0}),
                        llvm::makeArrayRef({theta}));
    inputQubit_0 =  new_inst_1.getResult(0);
    op.getResult(0).replaceAllUsesWith(inputQubit_0);
  }else if(num_qubit == 2){  //cry
    decompose_cry_gate(op,deadOps);
  }else{
    deadOps.emplace_back(op);
    
    mlir::OpBuilder rewriter(op);
    rewriter.setInsertionPointAfter(op);
    mlir::Value target_qubit = op.getOperand(controlled_qubit);
    mlir::Value input_qubit = op.getOperand(0);
    mlir::Value theta = op.getOperand(num_qubit);

    auto theta_d = qllvm::OP::tryGetConstAngle(theta);

    mlir::Type qubit_type = input_qubit.getType();
     // int last_control = controls.back();
    size_t last_c_num = controlled_qubit - 1;
    auto last_control = op.getOperand(last_c_num);

    mlir::Value angle_2_v = rewriter.create<mlir::ConstantOp>(
                    op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), theta_d/2.0));
    mlir::Value neg_angle_2_v = rewriter.create<mlir::ConstantOp>(
                    op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), -1*theta_d/2.0));

     // Step 1: apply C-RY(θ/2)
     // gate_list.extend(decompose_cry(angle / 2, last_control, target))
    auto new_inst_1 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                        op.getLoc(), llvm::makeArrayRef({qubit_type,qubit_type}),
                        "cry", llvm::makeArrayRef({last_control,target_qubit}),
                        llvm::makeArrayRef({angle_2_v}));
    last_control = new_inst_1.getResult(0);
    target_qubit = new_inst_1.getResult(1);
     // Step 2: apply C^{n-1}-X
     // gate_list.extend(decompose_mcx_to_cnot(other_controls, last_control))
    std::vector<mlir::Value> other_controls;
    std::vector<mlir::Type> qubit_type_v;
    std::unordered_map<size_t, mlir::Value> control_v;

    for(size_t i = 0; i <= controlled_qubit-2;i++){
      other_controls.emplace_back(op.getOperand(i));
      qubit_type_v.emplace_back(qubit_type);
      control_v[i] = op.getOperand(i);
    }

    other_controls.emplace_back(last_control);
    qubit_type_v.emplace_back(qubit_type);

    auto new_inst_2 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                        op.getLoc(), llvm::makeArrayRef(qubit_type_v),
                        "mcx", llvm::makeArrayRef(other_controls),
                        llvm::None);
    last_control = new_inst_2.getResult(last_c_num);
    for(size_t i = 0; i <= controlled_qubit-2;i++){
      control_v[i] = new_inst_2.getResult(i);
    }
      // Step 3: apply C-RY(-θ/2)
     // gate_list.extend(decompose_cry(-angle / 2, last_control, target))
    auto new_inst_3 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                        op.getLoc(), llvm::makeArrayRef({qubit_type,qubit_type}),
                        "cry", llvm::makeArrayRef({last_control,target_qubit}),
                        llvm::makeArrayRef({neg_angle_2_v}));
    last_control = new_inst_3.getResult(0);
    target_qubit = new_inst_3.getResult(1);

     // Step 4: again apply C^{n-1}-X
     // gate_list.extend(decompose_mcx_to_cnot(other_controls, last_control))
    other_controls.clear();
    for(int i = 0; i <= controlled_qubit-2;i++){
      other_controls.emplace_back(control_v[i]);
    }
    other_controls.emplace_back(last_control);
    auto new_inst_4 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                        op.getLoc(), llvm::makeArrayRef(qubit_type_v),
                        "mcx", llvm::makeArrayRef(other_controls),
                        llvm::None);
    for(int i = 0; i <= controlled_qubit-2;i++){
      control_v[i] = new_inst_4.getResult(i);
    }
    last_control = new_inst_4.getResult(controlled_qubit-1);
     // Step 5: recursively call C^{n-1}-RY(θ/2)
     // gate_list.extend(decompose_mcry(angle / 2, other_controls, target))
    other_controls.clear();
    for(int i = 0; i < controlled_qubit-1;i++){
      other_controls.emplace_back(control_v[i]);
    }
    other_controls.emplace_back(target_qubit);
    auto new_inst_5 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                        op.getLoc(), llvm::makeArrayRef(qubit_type_v),
                        "mcry", llvm::makeArrayRef(other_controls),
                        llvm::makeArrayRef({angle_2_v}));
    for(int i = 0; i <= controlled_qubit-2;i++){
      control_v[i] = new_inst_5.getResult(i);
    }
    target_qubit = new_inst_5.getResult(controlled_qubit-1);

    for(int i = 0; i <= controlled_qubit-2;i++){
      op.getResult(i).replaceAllUsesWith(control_v[i]);
    }

    op.getResult(controlled_qubit).replaceAllUsesWith(target_qubit);
    op.getResult(controlled_qubit-1).replaceAllUsesWith(last_control);

    decompose_cry_gate(new_inst_1,deadOps);
    decompose_mcx_gate(new_inst_2,deadOps);
    decompose_cry_gate(new_inst_3,deadOps);
    decompose_mcx_gate(new_inst_4,deadOps);
    decompose_mcry_gate(new_inst_5,deadOps);
  }
}

void decompose_mcrz_gate(mlir::quantum::ValueSemanticsInstOp &op,std::vector<mlir::quantum::ValueSemanticsInstOp> &deadOps){
  
  size_t num_qubit = op.getNumResults();
  size_t controlled_qubit = num_qubit - 1;

  if(num_qubit == 1){
    deadOps.emplace_back(op);
    mlir::OpBuilder rewriter(op);
    rewriter.setInsertionPointAfter(op);
    mlir::Value inputQubit_0 = op.getOperand(0);
    mlir::Value theta = op.getOperand(1);
    mlir::Type qubit_type = inputQubit_0.getType();

    auto new_inst_1 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                        op.getLoc(), llvm::makeArrayRef({qubit_type}),
                        "rz", llvm::makeArrayRef({inputQubit_0}),
                        llvm::makeArrayRef({theta}));
    inputQubit_0 =  new_inst_1.getResult(0);
    op.getResult(0).replaceAllUsesWith(inputQubit_0);
  }else if(num_qubit == 2){  //crz
    decompose_crz_gate(op,deadOps);
  }else{
    deadOps.emplace_back(op);
    
    mlir::OpBuilder rewriter(op);
    rewriter.setInsertionPointAfter(op);
    mlir::Value target_qubit = op.getOperand(controlled_qubit);
    mlir::Value input_qubit = op.getOperand(0);
    mlir::Value theta = op.getOperand(num_qubit);

    auto theta_d = qllvm::OP::tryGetConstAngle(theta);

    mlir::Type qubit_type = input_qubit.getType();
     // int last_control = controls.back();
    size_t last_c_num = controlled_qubit - 1;
    auto last_control = op.getOperand(last_c_num);

    mlir::Value angle_2_v = rewriter.create<mlir::ConstantOp>(
                    op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), theta_d/2.0));
    mlir::Value neg_angle_2_v = rewriter.create<mlir::ConstantOp>(
                    op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), -1*theta_d/2.0));

     // Step 1: apply C-Rz(θ/2)
     // gate_list.extend(decompose_crz(angle / 2, last_control, target))
    auto new_inst_1 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                        op.getLoc(), llvm::makeArrayRef({qubit_type,qubit_type}),
                        "crz", llvm::makeArrayRef({last_control,target_qubit}),
                        llvm::makeArrayRef({angle_2_v}));
    last_control = new_inst_1.getResult(0);
    target_qubit = new_inst_1.getResult(1);
     // Step 2: apply C^{n-1}-X
     // gate_list.extend(decompose_mcx_to_cnot(other_controls, last_control))
    std::vector<mlir::Value> other_controls;
    std::vector<mlir::Type> qubit_type_v;
    std::unordered_map<size_t, mlir::Value> control_v;

    for(size_t i = 0; i <= controlled_qubit-2;i++){
      other_controls.emplace_back(op.getOperand(i));
      qubit_type_v.emplace_back(qubit_type);
      control_v[i] = op.getOperand(i);
    }

    other_controls.emplace_back(last_control);
    qubit_type_v.emplace_back(qubit_type);

    auto new_inst_2 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                        op.getLoc(), llvm::makeArrayRef(qubit_type_v),
                        "mcx", llvm::makeArrayRef(other_controls),
                        llvm::None);
    last_control = new_inst_2.getResult(last_c_num);
    for(size_t i = 0; i <= controlled_qubit-2;i++){
      control_v[i] = new_inst_2.getResult(i);
    }
      // Step 3: apply C-RZ(-θ/2)
     // gate_list.extend(decompose_crz(-angle / 2, last_control, target))
    auto new_inst_3 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                        op.getLoc(), llvm::makeArrayRef({qubit_type,qubit_type}),
                        "crz", llvm::makeArrayRef({last_control,target_qubit}),
                        llvm::makeArrayRef({neg_angle_2_v}));
    last_control = new_inst_3.getResult(0);
    target_qubit = new_inst_3.getResult(1);

     // Step 4: again apply C^{n-1}-X
     // gate_list.extend(decompose_mcx_to_cnot(other_controls, last_control))
    other_controls.clear();
    for(int i = 0; i <= controlled_qubit-2;i++){
      other_controls.emplace_back(control_v[i]);
    }
    other_controls.emplace_back(last_control);
    auto new_inst_4 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                        op.getLoc(), llvm::makeArrayRef(qubit_type_v),
                        "mcx", llvm::makeArrayRef(other_controls),
                        llvm::None);
    for(int i = 0; i <= controlled_qubit-2;i++){
      control_v[i] = new_inst_4.getResult(i);
    }
    last_control = new_inst_4.getResult(controlled_qubit-1);
     // Step 5: recursively call C^{n-1}-Rz(θ/2)
     // gate_list.extend(decompose_mcry(angle / 2, other_controls, target))
    other_controls.clear();
    for(int i = 0; i < controlled_qubit-1;i++){
      other_controls.emplace_back(control_v[i]);
    }
    other_controls.emplace_back(target_qubit);
    auto new_inst_5 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                        op.getLoc(), llvm::makeArrayRef(qubit_type_v),
                        "mcrz", llvm::makeArrayRef(other_controls),
                        llvm::makeArrayRef({angle_2_v}));
    for(int i = 0; i <= controlled_qubit-2;i++){
      control_v[i] = new_inst_5.getResult(i);
    }
    target_qubit = new_inst_5.getResult(controlled_qubit-1);

    for(int i = 0; i <= controlled_qubit-2;i++){
      op.getResult(i).replaceAllUsesWith(control_v[i]);
    }

    op.getResult(controlled_qubit).replaceAllUsesWith(target_qubit);
    op.getResult(controlled_qubit-1).replaceAllUsesWith(last_control);

    decompose_crz_gate(new_inst_1,deadOps);
    decompose_mcx_gate(new_inst_2,deadOps);
    decompose_crz_gate(new_inst_3,deadOps);
    decompose_mcx_gate(new_inst_4,deadOps);
    decompose_mcrz_gate(new_inst_5,deadOps);
  }
}

 // gate dcx a, b { cx a, b; cx b, a; }
void decompose_dcx_gate(mlir::quantum::ValueSemanticsInstOp &op,std::vector<mlir::quantum::ValueSemanticsInstOp> &deadOps){
  deadOps.emplace_back(op);
  mlir::OpBuilder rewriter(op);
  rewriter.setInsertionPointAfter(op);

  std::vector<mlir::Value> param;
  mlir::Value inputQubit_0 = op.getOperand(0);
  mlir::Value inputQubit_1 = op.getOperand(1);

  mlir::Type qubit_type = inputQubit_0.getType();

  auto new_inst_1 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type,qubit_type}),
                      "cx", llvm::makeArrayRef({inputQubit_0,inputQubit_1}),
                      llvm::None);
  inputQubit_0 =  new_inst_1.getResult(0);               
  inputQubit_1 =  new_inst_1.getResult(1);               
  auto new_inst_2 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type,qubit_type}),
                      "cx", llvm::makeArrayRef({inputQubit_1,inputQubit_0}),
                      llvm::None);
  inputQubit_0 =  new_inst_2.getResult(1);               
  inputQubit_1 =  new_inst_2.getResult(0);   
  op.getResult(0).replaceAllUsesWith(inputQubit_0);
  op.getResult(1).replaceAllUsesWith(inputQubit_1);
}

 // gate rzx(theta) a, b { h b; cx a, b; u3(0,0,theta) b; cx a, b; h b;}
void decompose_rzx_gate(mlir::quantum::ValueSemanticsInstOp &op,std::vector<mlir::quantum::ValueSemanticsInstOp> &deadOps){
  deadOps.emplace_back(op);
  mlir::OpBuilder rewriter(op);
  rewriter.setInsertionPointAfter(op);

  std::vector<mlir::Value> param;
  mlir::Value inputQubit_0 = op.getOperand(0);
  mlir::Value inputQubit_1 = op.getOperand(1);
  mlir::Value theta = op.getOperand(2);
  mlir::Value theta_zero = rewriter.create<mlir::ConstantOp>(
                    op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), 0));

  mlir::Type qubit_type = inputQubit_0.getType();

  auto new_inst_1 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "h", llvm::makeArrayRef({inputQubit_1}),
                      llvm::None);             
  inputQubit_1 =  new_inst_1.getResult(0);               
  auto new_inst_2 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type,qubit_type}),
                      "cx", llvm::makeArrayRef({inputQubit_0,inputQubit_1}),
                      llvm::None);
  inputQubit_0 =  new_inst_2.getResult(0);               
  inputQubit_1 =  new_inst_2.getResult(1);

  param.emplace_back(theta_zero);
  param.emplace_back(theta_zero);
  param.emplace_back(theta);
  auto new_inst_3 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "u3", llvm::makeArrayRef({inputQubit_1}),
                      llvm::makeArrayRef(param));
  inputQubit_1 =  new_inst_3.getResult(0); 
  auto new_inst_4 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type,qubit_type}),
                      "cx", llvm::makeArrayRef({inputQubit_0,inputQubit_1}),
                      llvm::None);
  inputQubit_0 =  new_inst_4.getResult(0);               
  inputQubit_1 =  new_inst_4.getResult(1);
  auto new_inst_5 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "h", llvm::makeArrayRef({inputQubit_1}),
                      llvm::None);             
  inputQubit_1 =  new_inst_5.getResult(0);
  op.getResult(0).replaceAllUsesWith(inputQubit_0);
  op.getResult(1).replaceAllUsesWith(inputQubit_1);
}

 // gate ecr a, b { rzx(pi/4) a, b; x a; rzx(-pi/4) a, b;}
void decompose_ecr_gate(mlir::quantum::ValueSemanticsInstOp &op,std::vector<mlir::quantum::ValueSemanticsInstOp> &deadOps){
  deadOps.emplace_back(op);
  mlir::OpBuilder rewriter(op);
  rewriter.setInsertionPointAfter(op);

  std::vector<mlir::Value> param;
  mlir::Value inputQubit_0 = op.getOperand(0);
  mlir::Value inputQubit_1 = op.getOperand(1);
  mlir::Value theta_PI_4 = rewriter.create<mlir::ConstantOp>(
                    op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), M_PI_4));
  mlir::Value theta_neg_PI_4 = rewriter.create<mlir::ConstantOp>(
                    op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), -1*M_PI_4));
  mlir::Type qubit_type = inputQubit_0.getType();

  auto new_inst_1 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type,qubit_type}),
                      "rzx", llvm::makeArrayRef({inputQubit_0,inputQubit_1}),
                      llvm::makeArrayRef({theta_PI_4}));
  inputQubit_0 =  new_inst_1.getResult(0);               
  inputQubit_1 =  new_inst_1.getResult(1);
  auto new_inst_2 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "x", llvm::makeArrayRef({inputQubit_0}),
                      llvm::None);
  inputQubit_0 = new_inst_2.getResult(0);
  auto new_inst_3 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type,qubit_type}),
                      "rzx", llvm::makeArrayRef({inputQubit_1,inputQubit_0}),
                      llvm::makeArrayRef({theta_neg_PI_4}));
  inputQubit_0 =  new_inst_3.getResult(0);               
  inputQubit_1 =  new_inst_3.getResult(1);   
  op.getResult(0).replaceAllUsesWith(inputQubit_0);
  op.getResult(1).replaceAllUsesWith(inputQubit_1);
  decompose_rzx_gate(new_inst_1,deadOps);
  decompose_rzx_gate(new_inst_3,deadOps);
}

 // ryy(theta)
 //        ┌─────────┐                         ┌──────────┐
 //   q_0: ┤ Rx(π/2) ├──■───────────────────■──┤ Rx(-π/2) ├
 //        ├─────────┤┌─┴─┐┌───────────┐  ┌─┴─┐├──────────┤
 //   q_1: ┤ Rx(π/2) ├┤ X ├┤ Rz(theta) ├──┤ X ├┤ Rx(-π/2) ├
 //        └─────────┘└───┘└───────────┘  └───┘└──────────┘
void decompose_ryy_gate(mlir::quantum::ValueSemanticsInstOp &op,std::vector<mlir::quantum::ValueSemanticsInstOp> &deadOps){
  deadOps.emplace_back(op);
  mlir::OpBuilder rewriter(op);
  rewriter.setInsertionPointAfter(op);

  mlir::Value inputQubit_0 = op.getOperand(0);
  mlir::Value inputQubit_1 = op.getOperand(1);
  mlir::Value theta = op.getOperand(2);

  mlir::Value theta_PI_2 = rewriter.create<mlir::ConstantOp>(
                    op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), M_PI_2));
  mlir::Value neg_PI_2 = rewriter.create<mlir::ConstantOp>(
                    op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), -1*M_PI_2));
  mlir::Type qubit_type = inputQubit_0.getType();

  auto new_inst_1 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "rx", llvm::makeArrayRef({inputQubit_0}),
                      llvm::makeArrayRef({theta_PI_2}));
  inputQubit_0 = new_inst_1.getResult(0);

  auto new_inst_2 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "rx", llvm::makeArrayRef({inputQubit_1}),
                      llvm::makeArrayRef({theta_PI_2}));
  inputQubit_1 = new_inst_2.getResult(0);

  auto new_inst_3 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type,qubit_type}),
                      "cx", llvm::makeArrayRef({inputQubit_0,inputQubit_1}),
                      llvm::None);
  inputQubit_0 = new_inst_3.getResult(0);
  inputQubit_1 = new_inst_3.getResult(1);

  auto new_inst_4 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "rz", llvm::makeArrayRef({inputQubit_1}),
                      llvm::makeArrayRef({theta}));
  inputQubit_1 = new_inst_4.getResult(0);

  auto new_inst_5 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type,qubit_type}),
                      "cx", llvm::makeArrayRef({inputQubit_0,inputQubit_1}),
                      llvm::None);
  inputQubit_0 = new_inst_5.getResult(0);
  inputQubit_1 = new_inst_5.getResult(1);

  auto new_inst_6 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "rx", llvm::makeArrayRef({inputQubit_0}),
                      llvm::makeArrayRef({neg_PI_2}));
  inputQubit_0 = new_inst_6.getResult(0);

  auto new_inst_7 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "rx", llvm::makeArrayRef({inputQubit_1}),
                      llvm::makeArrayRef({neg_PI_2}));
  inputQubit_1 = new_inst_7.getResult(0);
  op.getResult(0).replaceAllUsesWith(inputQubit_0);
  op.getResult(1).replaceAllUsesWith(inputQubit_1);
}

 // gate ccz a,b,c { h c; ccx a,b,c; h c; }
void decompose_ccz_gate(mlir::quantum::ValueSemanticsInstOp &op,std::vector<mlir::quantum::ValueSemanticsInstOp> &deadOps){
  deadOps.emplace_back(op);
  mlir::OpBuilder rewriter(op);
  rewriter.setInsertionPointAfter(op);

  std::vector<mlir::Value> param;
  mlir::Value inputQubit_0 = op.getOperand(0);
  mlir::Value inputQubit_1 = op.getOperand(1);
  mlir::Value inputQubit_2 = op.getOperand(2);

  mlir::Type qubit_type = inputQubit_0.getType();
   // h c; 
  auto new_inst_1 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "h", llvm::makeArrayRef({inputQubit_2}),
                      llvm::None);
  inputQubit_2 =  new_inst_1.getResult(0);    

   // ccx a,b,c
  auto new_inst_2 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type,qubit_type,qubit_type}),
                      "ccx", llvm::makeArrayRef({inputQubit_0,inputQubit_1,inputQubit_2}),
                      llvm::None);                
  inputQubit_0 =  new_inst_2.getResult(0);
  inputQubit_1 =  new_inst_2.getResult(1);
  inputQubit_2 =  new_inst_2.getResult(2);
  
   // h c; 
  auto new_inst_3 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "h", llvm::makeArrayRef({inputQubit_2}),
                      llvm::None);
  inputQubit_2 =  new_inst_3.getResult(0);
  op.getResult(0).replaceAllUsesWith(inputQubit_0);
  op.getResult(1).replaceAllUsesWith(inputQubit_1);
  op.getResult(2).replaceAllUsesWith(inputQubit_2);
  decompose_ccx_gate(new_inst_2,deadOps);
}

 // gate r(θ, φ) a {u3(θ, φ - π/2, -φ + π/2) a;}
void decompose_r_gate(mlir::quantum::ValueSemanticsInstOp &op,std::vector<mlir::quantum::ValueSemanticsInstOp> &deadOps){
  deadOps.emplace_back(op);
  mlir::OpBuilder rewriter(op);
  rewriter.setInsertionPointAfter(op);

  std::vector<mlir::Value> param;
  mlir::Value inputQubit_0 = op.getOperand(0);
  mlir::Value theta_1 = op.getOperand(1);
  mlir::Value theta_2 = op.getOperand(2);
  auto theta2_d = qllvm::OP::tryGetConstAngle(theta_2);

  mlir::Value theta2_PI_2 = rewriter.create<mlir::ConstantOp>(
                    op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), theta2_d - M_PI_2));
  mlir::Value PI_2_theta2 = rewriter.create<mlir::ConstantOp>(
                    op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), -1*theta2_d + M_PI_2));
  mlir::Type qubit_type = inputQubit_0.getType();

  auto new_inst_1 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "u3", llvm::makeArrayRef({inputQubit_0}),
                      llvm::makeArrayRef({theta_1,theta2_PI_2,PI_2_theta2}));
  inputQubit_0 =  new_inst_1.getResult(0);    

  op.getResult(0).replaceAllUsesWith(inputQubit_0);
}

 // gate cs a,b { h b; cp(pi/2) a,b; h b; }
void decompose_cs_gate(mlir::quantum::ValueSemanticsInstOp &op,std::vector<mlir::quantum::ValueSemanticsInstOp> &deadOps){
  deadOps.emplace_back(op);
  mlir::OpBuilder rewriter(op);
  rewriter.setInsertionPointAfter(op);

  std::vector<mlir::Value> param;
  mlir::Value inputQubit_0 = op.getOperand(0);
  mlir::Value inputQubit_1 = op.getOperand(1);

  mlir::Value theta_PI_2 = rewriter.create<mlir::ConstantOp>(
                    op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), M_PI_2));

  mlir::Type qubit_type = inputQubit_0.getType();

  auto new_inst_1 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "h", llvm::makeArrayRef({inputQubit_1}),
                      llvm::None);
  inputQubit_1 =  new_inst_1.getResult(0);
  auto new_inst_2 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type,qubit_type}),
                      "cp", llvm::makeArrayRef({inputQubit_0,inputQubit_1}),
                      llvm::makeArrayRef({theta_PI_2}));
  inputQubit_0 =  new_inst_2.getResult(0);
  inputQubit_1 =  new_inst_2.getResult(1);
  auto new_inst_3 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "h", llvm::makeArrayRef({inputQubit_1}),
                      llvm::None);
  inputQubit_1 =  new_inst_3.getResult(0);

  op.getResult(0).replaceAllUsesWith(inputQubit_0);
  op.getResult(1).replaceAllUsesWith(inputQubit_1);
}

 // gate iswap a,b {
 //             s q[0];
 //             s q[1];
 //             h q[0];
 //             cx q[0],q[1];
 //             cx q[1],q[0];
 //             h q[1];
 //         }
void decompose_iswap_gate(mlir::quantum::ValueSemanticsInstOp &op,std::vector<mlir::quantum::ValueSemanticsInstOp> &deadOps){
  deadOps.emplace_back(op);
  mlir::OpBuilder rewriter(op);
  rewriter.setInsertionPointAfter(op);

  std::vector<mlir::Value> param;
  mlir::Value inputQubit_0 = op.getOperand(0);
  mlir::Value inputQubit_1 = op.getOperand(1);

  mlir::Type qubit_type = inputQubit_0.getType();

  auto new_inst_1 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "s", llvm::makeArrayRef({inputQubit_0}),
                      llvm::None);
  inputQubit_0 =  new_inst_1.getResult(0);
  auto new_inst_2 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "s", llvm::makeArrayRef({inputQubit_1}),
                      llvm::None);
  inputQubit_1 =  new_inst_2.getResult(0);
  auto new_inst_3 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "h", llvm::makeArrayRef({inputQubit_0}),
                      llvm::None);
  inputQubit_0 =  new_inst_3.getResult(0);
  auto new_inst_4 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type,qubit_type}),
                      "cx", llvm::makeArrayRef({inputQubit_0,inputQubit_1}),
                      llvm::None);
  inputQubit_0 =  new_inst_4.getResult(0);
  inputQubit_1 =  new_inst_4.getResult(1);
  auto new_inst_5 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type,qubit_type}),
                      "cx", llvm::makeArrayRef({inputQubit_1,inputQubit_0}),
                      llvm::None);
  inputQubit_0 =  new_inst_5.getResult(1);
  inputQubit_1 =  new_inst_5.getResult(0);
  auto new_inst_6 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "h", llvm::makeArrayRef({inputQubit_1}),
                      llvm::None);
  inputQubit_1 =  new_inst_6.getResult(0);

  op.getResult(0).replaceAllUsesWith(inputQubit_0);
  op.getResult(1).replaceAllUsesWith(inputQubit_1);
}

 // gate csdg a,b { h b; cp(-pi/2) a,b; h b; }
void decompose_csdg_gate(mlir::quantum::ValueSemanticsInstOp &op,std::vector<mlir::quantum::ValueSemanticsInstOp> &deadOps){
  deadOps.emplace_back(op);
  mlir::OpBuilder rewriter(op);
  rewriter.setInsertionPointAfter(op);

  std::vector<mlir::Value> param;
  mlir::Value inputQubit_0 = op.getOperand(0);
  mlir::Value inputQubit_1 = op.getOperand(1);

  mlir::Value theta_neg_PI_2 = rewriter.create<mlir::ConstantOp>(
                    op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), -1*M_PI_2));

  mlir::Type qubit_type = inputQubit_0.getType();

  auto new_inst_1 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "h", llvm::makeArrayRef({inputQubit_1}),
                      llvm::None);
  inputQubit_1 =  new_inst_1.getResult(0);
  auto new_inst_2 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type,qubit_type}),
                      "cp", llvm::makeArrayRef({inputQubit_0,inputQubit_1}),
                      llvm::makeArrayRef({theta_neg_PI_2}));
  inputQubit_0 =  new_inst_2.getResult(0);
  inputQubit_1 =  new_inst_2.getResult(1);
  auto new_inst_3 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "h", llvm::makeArrayRef({inputQubit_1}),
                      llvm::None);
  inputQubit_1 =  new_inst_3.getResult(0);

  op.getResult(0).replaceAllUsesWith(inputQubit_0);
  op.getResult(1).replaceAllUsesWith(inputQubit_1);
}

void decompose_ms_gate(mlir::quantum::ValueSemanticsInstOp &op,std::vector<mlir::quantum::ValueSemanticsInstOp> &deadOps){
  deadOps.emplace_back(op);
  mlir::OpBuilder rewriter(op);
  rewriter.setInsertionPointAfter(op);

  std::vector<mlir::Value> param;
  std::vector<mlir::Value> inputQubit;
  auto N = op.getNumResults(); // n-bit

  mlir::Value theta = op.getOperand(N);
  mlir::Value inputQubit_0 = op.getOperand(0);
  mlir::Type qubit_type = inputQubit_0.getType();

  for(int i = 0; i < N; i++){
    inputQubit.emplace_back(op.getOperand(i));
  }

  std::vector<mlir::quantum::ValueSemanticsInstOp> rxx_v;
  for(int i = 0; i < N; i++) {
    for (int j = i + 1; j < N;j++) {
      auto new_inst_1 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                    op.getLoc(), llvm::makeArrayRef({qubit_type,qubit_type}),
                    "rxx", llvm::makeArrayRef({inputQubit[i],inputQubit[j]}),
                    llvm::makeArrayRef({theta}));
      inputQubit[i] =  new_inst_1.getResult(0);
      inputQubit[j] =  new_inst_1.getResult(1);
      rxx_v.emplace_back(new_inst_1);
    }
  }
  for(int i = 0; i < N; i++){
    op.getResult(i).replaceAllUsesWith(inputQubit[i]);
  }
  for(auto &op_rxx: rxx_v){
    decompose_rxx_gate(op_rxx,deadOps);
  }
}

void decompose_rxy_gate(mlir::quantum::ValueSemanticsInstOp &op,std::vector<mlir::quantum::ValueSemanticsInstOp> &deadOps){
  deadOps.emplace_back(op);
  mlir::OpBuilder rewriter(op);
  rewriter.setInsertionPointAfter(op);

  std::vector<mlir::Value> param;
  mlir::Value inputQubit_1 = op.getOperand(0);
  mlir::Value phi_v = op.getOperand(1);
  mlir::Value theta_v = op.getOperand(2);
  auto phi_d = qllvm::OP::tryGetConstAngle(phi_v);

  mlir::Value theta_1 = rewriter.create<mlir::ConstantOp>(
                    op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), M_PI_2 - phi_d ));
  mlir::Value theta_2 = rewriter.create<mlir::ConstantOp>(
                      op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), phi_d - M_PI_2 ));
  mlir::Value theta_PI_2 = rewriter.create<mlir::ConstantOp>(
                        op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), M_PI_2 ));
  mlir::Value theta_neg_PI_2 = rewriter.create<mlir::ConstantOp>(
                          op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), -1*M_PI_2 ));  

  mlir::Type qubit_type = inputQubit_1.getType();

  auto new_inst_1 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "rz", llvm::makeArrayRef({inputQubit_1}),
                      llvm::makeArrayRef({theta_1}));
  inputQubit_1 =  new_inst_1.getResult(0);
  auto new_inst_2 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "rx", llvm::makeArrayRef({inputQubit_1}),
                      llvm::makeArrayRef({theta_PI_2}));
  inputQubit_1 =  new_inst_2.getResult(0);
  auto new_inst_3 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "rz", llvm::makeArrayRef({inputQubit_1}),
                      llvm::makeArrayRef({theta_v}));
  inputQubit_1 =  new_inst_3.getResult(0);
  auto new_inst_4 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "rx", llvm::makeArrayRef({inputQubit_1}),
                      llvm::makeArrayRef({theta_neg_PI_2}));
  inputQubit_1 =  new_inst_4.getResult(0);
  auto new_inst_5 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "rz", llvm::makeArrayRef({inputQubit_1}),
                      llvm::makeArrayRef({theta_2}));
  inputQubit_1 =  new_inst_5.getResult(0);

  op.getResult(0).replaceAllUsesWith(inputQubit_1);
}

void decompose_x2p_gate(mlir::quantum::ValueSemanticsInstOp &op,std::vector<mlir::quantum::ValueSemanticsInstOp> &deadOps){
  deadOps.emplace_back(op);
  mlir::OpBuilder rewriter(op);
  rewriter.setInsertionPointAfter(op);

  std::vector<mlir::Value> param;
  mlir::Value inputQubit_1 = op.getOperand(0);

  mlir::Value theta_PI_2 = rewriter.create<mlir::ConstantOp>(
                        op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), M_PI_2 ));

  mlir::Type qubit_type = inputQubit_1.getType();

  auto new_inst_1 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "rx", llvm::makeArrayRef({inputQubit_1}),
                      llvm::makeArrayRef({theta_PI_2}));
  inputQubit_1 =  new_inst_1.getResult(0);
  
  op.getResult(0).replaceAllUsesWith(inputQubit_1);
}

void decompose_x2m_gate(mlir::quantum::ValueSemanticsInstOp &op,std::vector<mlir::quantum::ValueSemanticsInstOp> &deadOps){
  deadOps.emplace_back(op);
  mlir::OpBuilder rewriter(op);
  rewriter.setInsertionPointAfter(op);

  std::vector<mlir::Value> param;
  mlir::Value inputQubit_1 = op.getOperand(0);

  mlir::Value theta_PI_2_neg = rewriter.create<mlir::ConstantOp>(
                        op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), -1*M_PI_2));

  mlir::Type qubit_type = inputQubit_1.getType();

  auto new_inst_1 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "rx", llvm::makeArrayRef({inputQubit_1}),
                      llvm::makeArrayRef({theta_PI_2_neg}));
  inputQubit_1 =  new_inst_1.getResult(0);
  
  op.getResult(0).replaceAllUsesWith(inputQubit_1);
}

void decompose_y2p_gate(mlir::quantum::ValueSemanticsInstOp &op,std::vector<mlir::quantum::ValueSemanticsInstOp> &deadOps){
  deadOps.emplace_back(op);
  mlir::OpBuilder rewriter(op);
  rewriter.setInsertionPointAfter(op);

  std::vector<mlir::Value> param;
  mlir::Value inputQubit_1 = op.getOperand(0);

  mlir::Value theta_PI_2 = rewriter.create<mlir::ConstantOp>(
                        op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), M_PI_2 ));

  mlir::Type qubit_type = inputQubit_1.getType();

  auto new_inst_1 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "ry", llvm::makeArrayRef({inputQubit_1}),
                      llvm::makeArrayRef({theta_PI_2}));
  inputQubit_1 =  new_inst_1.getResult(0);
  
  op.getResult(0).replaceAllUsesWith(inputQubit_1);
}

void decompose_y2m_gate(mlir::quantum::ValueSemanticsInstOp &op,std::vector<mlir::quantum::ValueSemanticsInstOp> &deadOps){
  deadOps.emplace_back(op);
  mlir::OpBuilder rewriter(op);
  rewriter.setInsertionPointAfter(op);

  std::vector<mlir::Value> param;
  mlir::Value inputQubit_1 = op.getOperand(0);

  mlir::Value theta_PI_2_neg = rewriter.create<mlir::ConstantOp>(
                        op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), -1*M_PI_2 ));

  mlir::Type qubit_type = inputQubit_1.getType();

  auto new_inst_1 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "ry", llvm::makeArrayRef({inputQubit_1}),
                      llvm::makeArrayRef({theta_PI_2_neg}));
  inputQubit_1 =  new_inst_1.getResult(0);
  
  op.getResult(0).replaceAllUsesWith(inputQubit_1);
}

void decompose_xy2p_gate(mlir::quantum::ValueSemanticsInstOp &op,std::vector<mlir::quantum::ValueSemanticsInstOp> &deadOps){
  deadOps.emplace_back(op);
  mlir::OpBuilder rewriter(op);
  rewriter.setInsertionPointAfter(op);

  std::vector<mlir::Value> param;
  mlir::Value inputQubit_1 = op.getOperand(0);
  mlir::Value theta = op.getOperand(1);
  auto theta_d = qllvm::OP::tryGetConstAngle(theta);

  mlir::Value theta_PI_2 = rewriter.create<mlir::ConstantOp>(
                        op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), M_PI_2 ));
  mlir::Value theta_1_v = rewriter.create<mlir::ConstantOp>(
                          op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), theta_d-M_PI_2 ));
  mlir::Value theta_2_v = rewriter.create<mlir::ConstantOp>(
                            op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), M_PI_2 - theta_d ));

  mlir::Type qubit_type = inputQubit_1.getType();

  auto new_inst_1 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "rz", llvm::makeArrayRef({inputQubit_1}),
                      llvm::makeArrayRef({theta_1_v}));
  inputQubit_1 =  new_inst_1.getResult(0);
  auto new_inst_2 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "ry", llvm::makeArrayRef({inputQubit_1}),
                      llvm::makeArrayRef({theta_PI_2}));
  inputQubit_1 =  new_inst_2.getResult(0);
  auto new_inst_3 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "rz", llvm::makeArrayRef({inputQubit_1}),
                      llvm::makeArrayRef({theta_2_v}));
  inputQubit_1 =  new_inst_3.getResult(0);
  
  op.getResult(0).replaceAllUsesWith(inputQubit_1);
}

void decompose_xy2m_gate(mlir::quantum::ValueSemanticsInstOp &op,std::vector<mlir::quantum::ValueSemanticsInstOp> &deadOps){
  deadOps.emplace_back(op);
  mlir::OpBuilder rewriter(op);
  rewriter.setInsertionPointAfter(op);

  std::vector<mlir::Value> param;
  mlir::Value inputQubit_1 = op.getOperand(0);
  mlir::Value theta = op.getOperand(1);
  auto theta_d = qllvm::OP::tryGetConstAngle(theta);

  mlir::Value theta_PI_2 = rewriter.create<mlir::ConstantOp>(
                        op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), M_PI_2 ));
  mlir::Value theta_1_v = rewriter.create<mlir::ConstantOp>(
                          op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), theta_d + M_PI_2 ));
  mlir::Value theta_2_v = rewriter.create<mlir::ConstantOp>(
                            op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), -1*(M_PI_2 + theta_d) ));

  mlir::Type qubit_type = inputQubit_1.getType();

  auto new_inst_1 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "rz", llvm::makeArrayRef({inputQubit_1}),
                      llvm::makeArrayRef({theta_1_v}));
  inputQubit_1 =  new_inst_1.getResult(0);
  auto new_inst_2 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "ry", llvm::makeArrayRef({inputQubit_1}),
                      llvm::makeArrayRef({theta_PI_2}));
  inputQubit_1 =  new_inst_2.getResult(0);
  auto new_inst_3 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      op.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "rz", llvm::makeArrayRef({inputQubit_1}),
                      llvm::makeArrayRef({theta_2_v}));
  inputQubit_1 =  new_inst_3.getResult(0);
  
  op.getResult(0).replaceAllUsesWith(inputQubit_1);
}

void DecomposemultiPass::runOnOperation() {
  //  std::cout << "DecomposemultiPass: " << std::endl;
  // gen_qasm(getOperation(),"before.qasm");
  std::vector<mlir::quantum::ValueSemanticsInstOp> deadOps;
  std::unordered_set<std::string> gate_set = {"u3","rx","ry","rz","x","y","z","cx","cy","cz","h","sdg","tdg","s","sx","p","cp","t","swap"};
  std::unordered_set<std::string> gate_to_decompose = {"u1","u2","u0","u","sxdg","ch","cy","cu3","cu1","cu","csx","ccx","cswap","crx","cry","crz","rxx","rzz","ryy","rccx","rc3x","c3x","c3sqrtx","c4x","cpx","cxz","mcx","mcp","mcrx","mcry","mcrz","dcx","rzx","ecr","ccz","r","cs","iswap","csdg","ms","rxy","X2P","X2M","Y2P","Y2M","XY2P","XY2M"};
  getOperation().walk([&](mlir::quantum::ValueSemanticsInstOp op) {
  
    auto inst_name = op.name().str();
    if(gate_to_decompose.find(inst_name) == gate_to_decompose.end() && gate_set.find(inst_name) == gate_set.end()){
      std::cout << "In DecomposemultiPass" << std::endl;
      throw std::runtime_error("Not support " + inst_name + " gate");
    }
    // if(gate_to_decompose.find(inst_name) != gate_to_decompose.end()){
    //   // return;
    //   std::cout << "gate name: " << inst_name << std::endl;
    // }

    if (inst_name == "u1" && op.getOperands().size() == 2) {
      decompose_u1_gate(op,deadOps);
    }else if(inst_name == "u2" && op.getOperands().size() == 3){
      decompose_u2_gate(op,deadOps);
    }else if((inst_name == "u0" || inst_name == "u" ) && op.getOperands().size() == 4){
      decompose_u_gate(op,deadOps);
    }else if(inst_name == "sxdg" && op.getOperands().size() == 1){
      decompose_sxdg_gate(op,deadOps);
    }else if(inst_name == "ch" && op.getOperands().size() == 2){
      decompose_ch_gate(op,deadOps);
    }else if(inst_name == "cy" && op.getOperands().size() == 2){
      decompose_cy_gate(op,deadOps);
    }else if(inst_name == "cu3" && op.getOperands().size() == 5){
      decompose_cu3_gate(op,deadOps);
    }else if(inst_name == "cu1" && op.getOperands().size() == 3){
      // std::cout << "decompose_cu1_gate: " << inst_name << std::endl;
      decompose_cu1_gate(op,deadOps);
    }else if(inst_name == "cu" && op.getOperands().size() == 6){
      decompose_cu_gate(op,deadOps);
    }else if(inst_name == "csx" && op.getOperands().size() == 2){
      decompose_csx_gate(op,deadOps);
    }else if(inst_name == "ccx" && op.getOperands().size() == 3){
      decompose_ccx_gate(op,deadOps);
    }else if(inst_name == "cswap" && op.getOperands().size() == 3){
      decompose_cswap_gate(op,deadOps);
    }else if(inst_name == "crx" && op.getOperands().size() == 3){
      decompose_crx_gate(op,deadOps);
    }else if(inst_name == "cry" && op.getOperands().size() == 3){
      decompose_cry_gate(op,deadOps);
    }else if(inst_name == "crz" && op.getOperands().size() == 3){
      decompose_crz_gate(op,deadOps);
    }else if(inst_name == "rxx" && op.getOperands().size() == 3){
      decompose_rxx_gate(op,deadOps);
    }else if(inst_name == "rzz" && op.getOperands().size() == 3){
      decompose_rzz_gate(op,deadOps);
    }else if(inst_name == "ryy" && op.getOperands().size() == 3){
      decompose_ryy_gate(op,deadOps);
    }else if(inst_name == "rccx" && op.getOperands().size() == 3){
      decompose_rccx_gate(op,deadOps);
    }else if(inst_name == "rc3x" && op.getOperands().size() == 4){
      decompose_rc3x_gate(op,deadOps);
    }else if(inst_name == "c3x" && op.getOperands().size() == 4){
      decompose_c3x_gate(op,deadOps);
    }else if(inst_name == "c3sqrtx" && op.getOperands().size() == 4){
      decompose_c3sqrtx_gate(op,deadOps);
    }else if(inst_name == "c4x" && op.getOperands().size() == 5){
      decompose_c4x_gate(op,deadOps);
    }else if(inst_name == "cpx" && op.getOperands().size() == 3){
      decompose_cpx_gate(op,deadOps);
    }else if(inst_name == "cxz" && op.getOperands().size() == 3){
      decompose_cxz_gate(op,deadOps);
    }else if(inst_name == "mcx"){
      decompose_mcx_gate(op,deadOps);
    }else if(inst_name == "mcp"){
      decompose_mcp_gate(op,deadOps);
    }else if(inst_name == "mcrx"){
      decompose_mcrx_gate(op,deadOps);
    }else if(inst_name == "mcry"){
      decompose_mcry_gate(op,deadOps);
    }else if(inst_name == "mcrz"){
      decompose_mcrz_gate(op,deadOps);
    }else if(inst_name == "dcx" && op.getOperands().size() == 2){
      decompose_dcx_gate(op,deadOps);
    }else if(inst_name == "rzx" && op.getOperands().size() == 3){
      decompose_rzx_gate(op,deadOps);
    }else if(inst_name == "ecr" && op.getOperands().size() == 2){
      decompose_ecr_gate(op,deadOps);
    }else if(inst_name == "ccz" && op.getOperands().size() == 3){
      decompose_ccz_gate(op,deadOps);
    }else if(inst_name == "r" && op.getOperands().size() == 3){
      decompose_r_gate(op,deadOps);
    }else if(inst_name == "cs" && op.getOperands().size() == 2){
      decompose_cs_gate(op,deadOps);
    }else if(inst_name == "iswap" && op.getOperands().size() == 2){
      decompose_iswap_gate(op,deadOps);
    }else if(inst_name == "csdg" && op.getOperands().size() == 2){
      decompose_csdg_gate(op,deadOps);
    }else if(inst_name == "ms"){
      decompose_ms_gate(op,deadOps);
    }else if(inst_name == "rxy" && op.getOperands().size() == 3){
      decompose_rxy_gate(op,deadOps);
    }else if(inst_name == "X2P" && op.getOperands().size() == 1){
      decompose_x2p_gate(op,deadOps);
    }else if(inst_name == "X2M" && op.getOperands().size() == 1){
      decompose_x2m_gate(op,deadOps);
    }else if(inst_name == "Y2P" && op.getOperands().size() == 1){
      decompose_y2p_gate(op,deadOps);
    }else if(inst_name == "Y2M" && op.getOperands().size() == 1){
      decompose_y2m_gate(op,deadOps);
    }else if(inst_name == "XY2P" && op.getOperands().size() == 2){
      decompose_xy2p_gate(op,deadOps);
    }else if(inst_name == "XY2M" && op.getOperands().size() == 2){
      decompose_xy2m_gate(op,deadOps);
    }else if(gate_to_decompose.find(inst_name) != gate_to_decompose.end()){
      throw std::runtime_error("Error: " + inst_name + " format is not correct");
    }
  });

  for (auto &op : deadOps) {
    op->dropAllUses();
    op.erase();
  }
  // getOperation().dump();
}
}