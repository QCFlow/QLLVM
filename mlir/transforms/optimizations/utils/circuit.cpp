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

#include "circuit.hpp"
using namespace mlir;
namespace qllvm {
namespace circuit{
// Modified 2026-11-19: explicit DFS postorder avoids overflowing the native
// stack on long wires (e.g. HWB11 with custom-pass statistics enabled).
int getDepth(mlir::quantum::ValueSemanticsInstOp &op) {
  auto cachedDepth = [](mlir::quantum::ValueSemanticsInstOp gate) {
    auto depth = gate->getAttrOfType<mlir::IntegerAttr>("depth");
    return depth ? static_cast<int>(depth.getInt()) : 0;
  };
  if (int depth = cachedDepth(op)) return depth;
  std::vector<std::pair<mlir::quantum::ValueSemanticsInstOp, bool>> pending;
  pending.emplace_back(op, false);
  while (!pending.empty()) {
    auto current = pending.back();
    pending.pop_back();
    auto gate = current.first;
    if (cachedDepth(gate)) continue;
    if (!current.second) {
      pending.emplace_back(gate, true);
      for (auto result : gate.getResults()) {
        if (!result.hasOneUse()) continue;
        if (auto next = dyn_cast<mlir::quantum::ValueSemanticsInstOp>(*result.user_begin()))
          if (!cachedDepth(next)) pending.emplace_back(next, false);
      }
      continue;
    }
    int depth = 0;
    for (auto result : gate.getResults()) {
      int childDepth = 0;
      if (result.hasOneUse()) {
        auto *user = *result.user_begin();
        if (auto next = dyn_cast<mlir::quantum::ValueSemanticsInstOp>(user))
          childDepth = cachedDepth(next);
        else if (auto inst = dyn_cast<mlir::quantum::InstOp>(user))
          if (inst.name() == "mz") childDepth = 1;
      }
      depth = std::max(depth, childDepth + 1);
    }
    mlir::OpBuilder rewriter(gate);
    gate->setAttr("depth", rewriter.getI32IntegerAttr(depth));
  }
  return cachedDepth(op);
}

void getGateCountAndTopOp(int &gate_count, std::vector<mlir::quantum::ValueSemanticsInstOp> &top_op, mlir::ModuleOp ops){
    // Modified for correctness: no stale handles after a rewriting pass.
    gate_count = 0;
    top_op.clear();
    ops.walk([&](mlir::quantum::ValueSemanticsInstOp op) {
      mlir::OpBuilder rewriter(op);
      op.getOperation()->setAttr(llvm::StringRef("depth"),mlir::IntegerAttr::get(rewriter.getI32Type(), 0));
        
      auto inst_name = op.name().str();
      gate_count++;
      
      auto first_operand = op.getOperand(0);
      int num_operand = op.getNumResults();
      
      auto firstoperand = first_operand.dyn_cast<mlir::OpResult>();
      auto operation = firstoperand ? firstoperand.getOwner() : nullptr;
      auto owner1 = dyn_cast_or_null<mlir::quantum::ValueSemanticsInstOp>(operation);
      if(!owner1){
        top_op.emplace_back(op);
        return;
      }
      if(num_operand > 1){
        for(int i = 1; i < num_operand; i++){
          auto secondoperand = op.getOperand(i).dyn_cast<mlir::OpResult>();
          auto secondoperation = secondoperand ? secondoperand.getOwner() : nullptr;
          auto owner2 = dyn_cast_or_null<mlir::quantum::ValueSemanticsInstOp>(secondoperation);
          if(!owner2){
            top_op.emplace_back(op);
          }
        }
      }
    });
    ops.walk([&](mlir::quantum::InstOp op) {
      if("mz"==op.name()){
        gate_count += 1;
      }
    });
}

void getGateCount(int &gate_count, mlir::ModuleOp ops){
    ops.walk([&](mlir::quantum::ValueSemanticsInstOp op) { 
      gate_count++;
    });
    ops.walk([&](mlir::quantum::InstOp op) {
      if("mz"==op.name()){
        gate_count += 1;
      }
    });
}

int getDepth(mlir::ModuleOp ops){
  std::vector<mlir::quantum::ValueSemanticsInstOp> top_op;
  int gate_count = 0;
  int tmp_depth = 0;
  int depth = 0;
  getGateCountAndTopOp(gate_count, top_op, ops);
  for (auto &op : top_op) {
    tmp_depth = circuit::getDepth(op);
    depth = tmp_depth > depth ? tmp_depth : depth;
  }
  return depth;
}

int getGateNum(mlir::ModuleOp ops){
  int gate_count = 0; 
  ops.walk([&](mlir::quantum::ValueSemanticsInstOp op) { 
    gate_count++;
  });
  ops.walk([&](mlir::quantum::InstOp op) {
    if("mz"==op.name()){
      gate_count += 1;
    }
  });
  return gate_count;
}

}
}