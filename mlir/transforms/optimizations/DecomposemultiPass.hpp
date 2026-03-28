#pragma once
#include "Quantum/QuantumOps.h"
#include "mlir/Pass/Pass.h"
#include "mlir/Pass/PassManager.h"
#include "mlir/Target/LLVMIR.h"
#include "mlir/Transforms/DialectConversion.h"
#include "mlir/Transforms/Passes.h"

using namespace mlir;

namespace qllvm {
struct DecomposemultiPass
    : public PassWrapper<DecomposemultiPass, OperationPass<ModuleOp>> {
  void getDependentDialects(DialectRegistry &registry) const override;
  void runOnOperation() final;
  DecomposemultiPass() {}
  DecomposemultiPass(std::map<std::string, bool> bool_args,int &opt_count, int &opt_depth, int &cir_depth, int &zero_count, int &enable, int &pass_count) {
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
  std::string passname = "DecomposemultiPass";
};
} // namespace qllvm