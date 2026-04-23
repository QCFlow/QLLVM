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
namespace OP{
  std::pair<std::string, int64_t>  getbit_from_extractQubitOp(mlir::quantum::ExtractQubitOp op);
  std::pair<std::string, int64_t>  getbit_from_muti_valueSemanticsInstOp(mlir::quantum::ValueSemanticsInstOp op,int index);
  std::pair<std::string, int64_t>  getbit_from_valueSemanticsInstOp(mlir::quantum::ValueSemanticsInstOp op);
  std::vector<std::pair<std::string, int64_t> > getbit_from_muti_valueSemanticsInstOp(mlir::quantum::ValueSemanticsInstOp op);
  double tryGetConstAngle(mlir::Value theta_var);
}//namespace OP
} // namespace qllvm
