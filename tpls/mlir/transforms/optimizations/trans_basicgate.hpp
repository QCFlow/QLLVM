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