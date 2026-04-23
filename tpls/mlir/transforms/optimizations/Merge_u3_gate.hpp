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
#include <unordered_set> 


using namespace mlir;

namespace qllvm {
// We make each optimization routine into its own Pass so that
// option such as `--print-ir-before-all` can be used to inspect
// each pass independently.
// TODO: make this a FunctionPass
struct Merge_u3_gate
    : public PassWrapper<Merge_u3_gate, OperationPass<ModuleOp>> {
  void getDependentDialects(DialectRegistry &registry) const override;
  void runOnOperation() final;
  Merge_u3_gate() {}
  ~Merge_u3_gate() {}
  Merge_u3_gate(std::map<std::string, bool> bool_args,int &opt_count, int &opt_depth, int &cir_depth, int &zero_count, int &enable, int &pass_count) {
    if(bool_args.find("pass_effect") != bool_args.end()){
      printCountAndDepth = true;
      p = &opt_count;
      q = &opt_depth;
      c_d = &cir_depth;
    }
    if(bool_args.find("syn_opt") != bool_args.end()||bool_args.find("customPassSequence") != bool_args.end()){
      syn = true;
      o = &zero_count;
      e = &enable;
      c_d = &cir_depth;
    }
    if(bool_args.find("pass_count") != bool_args.end()){
        c = &pass_count;
        f = true;
    }
  }
private:
  bool printCountAndDepth = false;
  bool syn = false;
  bool f = false;
  int *p = nullptr;
  int *q = nullptr;
  int *o = nullptr;
  int *e = nullptr;
  int *c = nullptr;
  int *c_d = nullptr;
  int before_gate_count = 0;
  int before_circuit_depth = 0;
  int after_gate_count = 0;
  int after_circuit_depth = 0;
  std::vector<mlir::quantum::ValueSemanticsInstOp> top_op;
  std::string passname = "Merge_u3_gate";
};
} // namespace qllvm