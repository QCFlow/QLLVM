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
#include "mlir/Dialect/LLVMIR/LLVMDialect.h"
#include "mlir/Dialect/StandardOps/IR/Ops.h"
#include "mlir/IR/Matchers.h"
#include "mlir/IR/PatternMatch.h"
#include "mlir/Pass/Pass.h"
#include "mlir/Pass/PassManager.h"
#include "mlir/Target/LLVMIR.h"
#include "mlir/Transforms/DialectConversion.h"
#include "mlir/Transforms/Passes.h"
#include "Quantum/QuantumOps.h"
using namespace mlir;
namespace qllvm {
namespace circuit{
int getDepth(mlir::quantum::ValueSemanticsInstOp & op);
void getGateCountAndTopOp(int &gate_count, std::vector<mlir::quantum::ValueSemanticsInstOp> &top_op, mlir::ModuleOp ops);
void getGateCount(int &gate_count, mlir::ModuleOp ops);
int getDepth(mlir::ModuleOp ops);
int getGateNum(mlir::ModuleOp ops);
}//namespace circuit
} // namespace qllvm