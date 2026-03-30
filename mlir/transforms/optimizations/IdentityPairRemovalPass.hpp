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
struct SingleQubitIdentityPairRemovalPass
    : public PassWrapper<SingleQubitIdentityPairRemovalPass,
                         OperationPass<ModuleOp>> {
  void getDependentDialects(DialectRegistry &registry) const override;
  void runOnOperation() final;
  SingleQubitIdentityPairRemovalPass() {}
  SingleQubitIdentityPairRemovalPass(std::map<std::string, bool> bool_args,int &opt_count, int &opt_depth, int &cir_depth, int &zero_count, int &enable, int &pass_count) {
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

protected:
  static inline const std::map<std::string, std::string> search_gates{
      {"x", "x"},   {"y", "y"},   {"z", "z"},   {"h", "h"},
      {"t", "tdg"}, {"tdg", "t"}, {"s", "sdg"}, {"sdg", "s"}};
  bool should_remove(std::string name1, std::string name2) const;
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
  std::string passname = "SingleQubitIdentityPairRemovalPass";
};

struct CNOTIdentityPairRemovalPass
    : public PassWrapper<CNOTIdentityPairRemovalPass, OperationPass<ModuleOp>> {
  void getDependentDialects(DialectRegistry &registry) const override;
  void runOnOperation() final;
  CNOTIdentityPairRemovalPass() {}
  CNOTIdentityPairRemovalPass(std::map<std::string, bool> bool_args,int &opt_count, int &opt_depth, int &cir_depth, int &zero_count, int &enable, int &pass_count) {
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
  std::string passname = "CNOTIdentityPairRemovalPass";
};

// Remove duplicate reset:
// 2 consecutive resets on a single qubit line => remove one.
struct DuplicateResetRemovalPass
    : public PassWrapper<DuplicateResetRemovalPass, OperationPass<ModuleOp>> {
  void getDependentDialects(DialectRegistry &registry) const override;
  void runOnOperation() final;
  DuplicateResetRemovalPass() {}
  DuplicateResetRemovalPass(std::map<std::string, bool> bool_args,int &opt_count, int &opt_depth, int &cir_depth, int &zero_count, int &enable, int &pass_count) {
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
  std::string passname = "DuplicateResetRemovalPass";
};
} // namespace qllvm