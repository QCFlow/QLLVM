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

#include "Check_gate.hpp"
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
#include <string>
#include <utility>
#include <cassert>
#include "utils/circuit.hpp"
#include "utils/op.hpp"
#include "utils/gate_matrix.hpp"
#include "utils/get_matrix.hpp"
#include "CommutativeCancellationPass.hpp"
#include <Eigen/Dense>
#include <eigen3/unsupported/Eigen/KroneckerProduct>
#include <eigen3/unsupported/Eigen/MatrixFunctions>
#include <unordered_map>
#include <tr1/unordered_map>
#include <cmath>
#include <algorithm>
#include <cstdio>

namespace qllvm {
using namespace std::complex_literals;
void Check_gate::getDependentDialects(DialectRegistry &registry) const {
  registry.insert<LLVM::LLVMDialect>();
}

void Check_gate::runOnOperation() {
  getOperation().walk([&](mlir::quantum::ValueSemanticsInstOp op) {
    auto mat = qllvm::matrix::getGateMat(op,0);
  });

}
}
// namespace qllvm