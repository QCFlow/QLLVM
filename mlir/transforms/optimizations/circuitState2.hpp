#pragma once
#include "Quantum/QuantumOps.h"
#include "mlir/Pass/Pass.h"
#include "mlir/Pass/PassManager.h"
#include "mlir/Target/LLVMIR.h"
#include "mlir/Transforms/DialectConversion.h"
#include "mlir/Transforms/Passes.h"

using namespace mlir;

namespace qllvm {
struct circuitState2
    : public PassWrapper<circuitState2, OperationPass<ModuleOp>> {
  void getDependentDialects(DialectRegistry &registry) const override;
  void runOnOperation() final;
  circuitState2() {}
  circuitState2(std::map<std::string, bool> bool_args,int &pass_count) {
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
  bool test = false;
  bool syn = false;
  bool random_seq = false;
  int gateCount = 0;
  int ciruitDepth = 0;
};

} // namespace qllvm
