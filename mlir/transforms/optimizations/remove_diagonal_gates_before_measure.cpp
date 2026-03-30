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
#include "remove_diagonal_gates_before_measure.hpp"
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
void remove_diagonal_gates_before_measure::getDependentDialects(DialectRegistry &registry) const {
  registry.insert<LLVM::LLVMDialect>();
}

int remove_diagonal_gates_before_measure::get_diagonal_gates_before_measure(std::vector<mlir::quantum::ValueSemanticsInstOp> &deadOps, mlir::quantum::ValueSemanticsInstOp op, int i){
    if(std::find_if(
            diagonal_gates.begin(), diagonal_gates.end(),
            [&](const std::string &gate) {
            return op.name().str() == gate;
            }) == diagonal_gates.end()){
        return 0;
    }

    deadOps.emplace_back(op);
    auto operand = op.getOperand(i);
    auto operation = operand.dyn_cast_or_null<mlir::OpResult>().getOwner();
    auto owner = dyn_cast_or_null<mlir::quantum::ValueSemanticsInstOp>(operation);
    if(owner){
        if(owner.getNumResults() > 1 && owner.getResult(1) == operand){
            return get_diagonal_gates_before_measure(deadOps,owner,1);
        }else if(owner.getNumResults() == 1){
            return get_diagonal_gates_before_measure(deadOps,owner,0);
        }else{
            return 0;
        }
    }else{
        return 1;
    }
}

bool remove_diagonal_gates_before_measure::recursionJudgeSingleGate(mlir::quantum::ValueSemanticsInstOp &op){

  if (op.getNumResults() > 1 || std::find_if(
      diagonal_gates.begin(), diagonal_gates.end(),[&](const std::string &gate) {
      return op.name().str() == gate;
      }) == diagonal_gates.end()) return false;
  auto result_var = op.getResult(0);
  auto def_op = *result_var.user_begin();
  if (def_op) {
    if (auto VSI_def_op = dyn_cast_or_null<mlir::quantum::ValueSemanticsInstOp>(def_op)){
      return recursionJudgeSingleGate(VSI_def_op);
    }
  }
  return true;
}


void remove_diagonal_gates_before_measure::runOnOperation() {

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

  std::vector<mlir::quantum::ValueSemanticsInstOp> measures;
  getOperation().walk([&](mlir::quantum::InstOp op) {
    if("mz"==op.name()){
      measures.emplace_back(op);
    }
  });

  for(auto &op : measures){
    std::vector<mlir::quantum::ValueSemanticsInstOp> deadOps;
    auto operand = op.getOperand(0);
    auto operation = operand.dyn_cast_or_null<mlir::OpResult>().getOwner();
    auto owner = dyn_cast_or_null<mlir::quantum::ValueSemanticsInstOp>(operation);
    int i = 0;
    if(owner){
      deadOps.clear();
      if(std::find_if(
                diagonal_gates.begin(), diagonal_gates.end(),
                [&](const std::string &gate) {
                return owner.name().str() == gate;
                }) != diagonal_gates.end()){
            
            if(owner.getNumResults() > 1 && owner.getResult(1) == operand){
                i = get_diagonal_gates_before_measure(deadOps,owner,1);
            }else if(owner.getNumResults() == 1){
                i = get_diagonal_gates_before_measure(deadOps,owner,0);
            }else{
                continue;
            }
        }else{
            continue;
        }
    }else{
        break;
    }

    mlir::quantum::ValueSemanticsInstOp last_twobit_diagonal_gate;
    std::vector<mlir::quantum::ValueSemanticsInstOp> deadOps_new;
    int num = 0;
    for(num = 0;num < deadOps.size();num++){
      if(deadOps[num].getNumResults() == 2){
        last_twobit_diagonal_gate = deadOps[num];
        auto return_value1 = last_twobit_diagonal_gate.getResult(0);
        if(return_value1.hasOneUse()){
          auto user1 = *return_value1.user_begin();
          auto next_inst1 = dyn_cast_or_null<mlir::quantum::ValueSemanticsInstOp>(user1);
          if(!next_inst1){
            break;
          }
          if(!recursionJudgeSingleGate(next_inst1)){
            for(int t = 0;t < num;t++){
              deadOps_new.emplace_back(deadOps[t]);
            }
            deadOps = deadOps_new;
          }
        }
        break;
      }
    }
    if(deadOps.size() == 0){
      continue;
    }

    auto front_diagonal = deadOps.back();
    auto back_diagonal = deadOps.front();
    
    for(auto &d :deadOps){
      if(d.getNumResults()>1){
          d.getResult(0).replaceAllUsesWith(d.getOperand(0));
      }
    }

    if(back_diagonal.getNumResults()>1 && front_diagonal.getNumResults()>1){
        back_diagonal.getResult(1).replaceAllUsesWith(front_diagonal.getOperand(1));
    }else if(back_diagonal.getNumResults()>1 && front_diagonal.getNumResults() == 1){
        back_diagonal.getResult(1).replaceAllUsesWith(front_diagonal.getOperand(0));
    }else if(back_diagonal.getNumResults() == 1 && front_diagonal.getNumResults() > 1){
        back_diagonal.getResult(0).replaceAllUsesWith(front_diagonal.getOperand(1));
    }else{
      back_diagonal.getResult(0).replaceAllUsesWith(front_diagonal.getOperand(0));
    }
    
    for (auto &op1 : deadOps) {
        op1->dropAllUses();
        op1.erase();
    }
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
}