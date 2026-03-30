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
struct remove_diagonal_gates_before_measure
    : public PassWrapper<remove_diagonal_gates_before_measure, OperationPass<ModuleOp>> {
  void getDependentDialects(DialectRegistry &registry) const override;
  void runOnOperation() final;
  remove_diagonal_gates_before_measure() {}
  remove_diagonal_gates_before_measure(std::map<std::string, bool> bool_args,int &opt_count, int &opt_depth, int &cir_depth, int &zero_count, int &enable, int &pass_count) {
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
  int *p = nullptr;
  int *q = nullptr;
  int *o = nullptr;
  int *e = nullptr;
  int *c = nullptr;
  bool f = false;
  int *c_d = nullptr;
  int before_gate_count = 0;
  int before_circuit_depth = 0;
  int after_gate_count = 0;
  int after_circuit_depth = 0;
  std::vector<mlir::quantum::ValueSemanticsInstOp> top_op;
  std::string passname = "remove_diagonal_gates_before_measure";
  static inline const std::vector<std::string> diagonal_gates = {"rz", "z", "t", "s", "tdg", "sdg", "u1", "p", "cz", "crz", "cu1", "cphase", "rzz"};
  int get_diagonal_gates_before_measure(std::vector<mlir::quantum::ValueSemanticsInstOp> &deadOps, mlir::quantum::ValueSemanticsInstOp op, int i);
  bool recursionJudgeSingleGate(mlir::quantum::ValueSemanticsInstOp &op);
};
} // namespace qllvm