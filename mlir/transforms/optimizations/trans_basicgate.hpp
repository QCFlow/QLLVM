#pragma once
#include "Quantum/QuantumOps.h"
#include "mlir/Pass/Pass.h"
#include "mlir/Pass/PassManager.h"
#include "mlir/Target/LLVMIR.h"
#include "mlir/Transforms/DialectConversion.h"
#include "mlir/Transforms/Passes.h"
#include <unordered_map>
#include <tr1/unordered_map>
#include <iostream>
#include <unordered_set> 


using namespace mlir;

namespace qllvm {
using pauli_decomp_t = std::pair<std::string, double>;
struct trans_basicgate
    : public PassWrapper<trans_basicgate, OperationPass<ModuleOp>> {
  void getDependentDialects(DialectRegistry &registry) const override;
  void runOnOperation() final;
  trans_basicgate() {}
  ~trans_basicgate() {}

  trans_basicgate(std::unordered_set<std::string> basicgate) {
    basic_gate = basicgate;
  }

  private:

  std::unordered_set<std::string> basic_gate={"rx","ry","rz","h","cz"};//{rx,ry,rz,h,cz},{rx,ry,rz,cx},{su2,cz,x,y,z}

  std::string passname = "trans_basicgate";
  };
}