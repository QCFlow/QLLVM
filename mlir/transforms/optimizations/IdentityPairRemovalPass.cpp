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
#include "IdentityPairRemovalPass.hpp"
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

namespace qllvm {
void SingleQubitIdentityPairRemovalPass::getDependentDialects(
    DialectRegistry &registry) const {
  registry.insert<LLVM::LLVMDialect>();
}

void SingleQubitIdentityPairRemovalPass::runOnOperation() {
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
    if (std::find(deadOps.begin(), deadOps.end(), op) != deadOps.end()) {
      // Skip this op since it was merged (forward search)
      return;
    }

    auto inst_name = op.name();
    auto return_value = *op.result().begin();
    if (return_value.hasOneUse()) {
      // get that one user
      auto user = *return_value.user_begin();
      // cast to a inst op
      if (auto next_inst =
              dyn_cast_or_null<mlir::quantum::ValueSemanticsInstOp>(user)) {
        // check that it is one of our known id pairs
        if (should_remove(next_inst.name().str(), inst_name.str())) {
          // need to get users of next_inst and point them to use
          // op.getOperands
          (*next_inst.result_begin()).replaceAllUsesWith(op.getOperand(0));
          deadOps.emplace_back(op);
          deadOps.emplace_back(next_inst);
        }
      }
    }
  });

  for (auto &op : deadOps) {
    op->dropAllUses();
    op.erase();
  }
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
      if(before_gate_count-after_gate_count == 0 && before_circuit_depth-after_circuit_depth == 0){
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

bool SingleQubitIdentityPairRemovalPass::should_remove(
    std::string name1, std::string name2) const {
  if (search_gates.count(name1)) {
    return search_gates.at(name1) == name2;
  }
  return false;
}
void CNOTIdentityPairRemovalPass::getDependentDialects(
    DialectRegistry &registry) const {
  registry.insert<LLVM::LLVMDialect>();
}
void CNOTIdentityPairRemovalPass::runOnOperation() {
  // std::cout << "CNOTIdentityPairRemovalPass: " << std::endl;
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
    if (std::find(deadOps.begin(), deadOps.end(), op) != deadOps.end()) {
      // Skip this op since it was merged (forward search)
      return;
    }

    auto inst_name = op.name();

    if (inst_name != "cnot" && inst_name != "cx") {
      return;
    }
    if(op.getOperands().size() != 2){
      throw std::runtime_error("CNOT operation must have 2 results");
    }
    // Get the src ret qubit and the tgt ret qubit
    auto src_return_val = op.result().front();
    auto tgt_return_val = op.result().back();

    // Make sure they are used
    if (src_return_val.hasOneUse() && tgt_return_val.hasOneUse()) {

      // get the users of these values
      auto src_user = *src_return_val.user_begin();
      auto tgt_user = *tgt_return_val.user_begin();

      // Cast them to InstOps
      auto next_inst =
          dyn_cast_or_null<mlir::quantum::ValueSemanticsInstOp>(src_user);
      auto tmp_tgt =
          dyn_cast_or_null<mlir::quantum::ValueSemanticsInstOp>(tgt_user);
      if (!next_inst || !tmp_tgt) {
        // not inst ops
        return;
      }

      // We want the case where src_user and tgt_user are the same
      if (next_inst.getOperation() != tmp_tgt.getOperation()) {
        return;
      }

      // Need src_return_val to map to next_inst operand 0,
      // and tgt_return_val to map to next_inst operand 1.
      // if not drop out
      if (next_inst.getOperand(0) != src_return_val &&
          next_inst.getOperand(1) != tgt_return_val) {
        return;
      }

      // Next instruction must be a CNOT to merge
      if (next_inst.name() != "cnot" && next_inst.name() != "cx") {
        return;
      }

      // They are the same operation, a cnot
      // so we have cnot src, tgt | cnot src, tgt
      auto next_inst_result_0 = next_inst.result().front();
      auto next_inst_result_1 = next_inst.result().back();
      next_inst_result_0.replaceAllUsesWith(op.getOperand(0));
      next_inst_result_1.replaceAllUsesWith(op.getOperand(1));

      // Remove the identity pair
      deadOps.emplace_back(op);
      deadOps.emplace_back(src_user);
    }
  });

  for (auto &op : deadOps) {
    op->dropAllUses();
    op.erase();
  }

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
      if(before_gate_count-after_gate_count == 0 && before_circuit_depth-after_circuit_depth == 0){
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

void DuplicateResetRemovalPass::getDependentDialects(
    DialectRegistry &registry) const {
  registry.insert<LLVM::LLVMDialect>();
}

void DuplicateResetRemovalPass::runOnOperation() {
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
    if (std::find(deadOps.begin(), deadOps.end(), op) != deadOps.end()) {
      // Skip this op since it was deleted (forward search)
      return;
    }
    auto inst_name = op.name();
    
    if (inst_name != "reset") {
      return;
    }

    auto return_value = *op.result().begin();
    if (return_value.hasOneUse()) {
      // get that one user
      auto user = *return_value.user_begin();
      // cast to a inst op
      if (auto next_inst =
              dyn_cast_or_null<mlir::quantum::ValueSemanticsInstOp>(user)) {
        if (next_inst.name().str() == "reset") {
          // Two resets in a row:
          // Chain the input -> output and mark this second reset to delete:
          (*next_inst.result_begin())
              .replaceAllUsesWith(next_inst.getOperand(0));
          deadOps.emplace_back(next_inst);
        }
      }
    }
  });

  for (auto &op : deadOps) {
    op->dropAllUses();
    op.erase();
  }
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
      if(before_gate_count-after_gate_count == 0 && before_circuit_depth-after_circuit_depth == 0){
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
