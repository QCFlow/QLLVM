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
#pragma once
#include "Quantum/QuantumOps.h"
#include "mlir/Pass/Pass.h"
#include "mlir/Pass/PassManager.h"
#include "mlir/Target/LLVMIR.h"
#include "mlir/Transforms/DialectConversion.h"
#include "mlir/Transforms/Passes.h"

using namespace mlir;

namespace qllvm {
struct circuitState
    : public PassWrapper<circuitState, OperationPass<ModuleOp>> {
  void getDependentDialects(DialectRegistry &registry) const override;
  void runOnOperation() final;
  circuitState() {}
  circuitState(std::map<std::string, bool> bool_args,int &pass_count) {
    if(bool_args.find("pass_count") != bool_args.end()){
      c = &pass_count;
      flag = true;
    }
    if(bool_args.find("test") != bool_args.end()){
      test = true;
    }
    if(bool_args.find("syn_opt") != bool_args.end()){
      syn = true;
    }
    if(bool_args.find("random_seq") != bool_args.end()){
      random_seq = true;
    }
  }

private:
  std::vector<mlir::quantum::ValueSemanticsInstOp> top_op;
  int *c = nullptr;
  bool flag = false;
  bool syn = false;
  bool test = false;
  bool random_seq = false;
  int gateCount = 0;
  int ciruitDepth = 0;
};

} // namespace qllvm
