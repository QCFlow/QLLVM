/*******************************************************************************
 * Copyright (c) 2018-, UT-Battelle, LLC.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the MIT License 
 * which accompanies this distribution. 
 *
 * Contributors:
 *   Alexander J. McCaskey - initial API and implementation
 *   Thien Nguyen - implementation
 *******************************************************************************/
#pragma once
#include "Quantum/QuantumOps.h"
#include "mlir/Pass/Pass.h"
#include "mlir/Pass/PassManager.h"
#include "mlir/Target/LLVMIR.h"
#include "mlir/Transforms/DialectConversion.h"
#include "mlir/Transforms/Passes.h"

using namespace mlir;

namespace qllvm {
struct RemoveUnusedQIRCallsPass
    : public PassWrapper<RemoveUnusedQIRCallsPass,
                         OperationPass<ModuleOp>> {
  void getDependentDialects(DialectRegistry &registry) const override;
  void runOnOperation() final;
  RemoveUnusedQIRCallsPass() {}
  RemoveUnusedQIRCallsPass(std::string fn) {
    file_name = fn;
  }
  RemoveUnusedQIRCallsPass(std::map<std::string, bool> bool_args,int &pass_count, int &opt_depth, int &cir_depth,std::string fn) {
    file_name = fn;
    if(bool_args.find("pass_count") != bool_args.end()){
      c = &pass_count;
      f = true;
    }
    if(bool_args.find("pass_effect") != bool_args.end()){
      printCountAndDepth = true;
      q = &opt_depth;
      c_d = &cir_depth;
    }
  }
private:
  int *c = nullptr;
  bool f = false;
  int *q = nullptr;
  int *c_d = nullptr;
  bool printCountAndDepth = false;
  int before_gate_count = 0;
  int before_circuit_depth = 0;
  int after_gate_count = 0;
  int after_circuit_depth = 0;
  std::string file_name = "";
  std::vector<mlir::quantum::ValueSemanticsInstOp> top_op;
  std::string passname = "RemoveUnusedQIRCalls";
};
} // namespace qllvm
