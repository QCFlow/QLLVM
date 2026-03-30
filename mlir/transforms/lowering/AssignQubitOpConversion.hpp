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
 *
 * Modified by QCFlow (2026) for QLLVM project.
 */
#pragma once
#include "quantum_to_llvm.hpp"

namespace qllvm {
class AssignQubitOpConversion : public ConversionPattern {
protected:
  std::map<std::string, mlir::Value> &variables;

public:
  // CTor: store seen variables
  explicit AssignQubitOpConversion(MLIRContext *context,
                                   std::map<std::string, mlir::Value> &vars)
      : ConversionPattern(mlir::quantum::AssignQubitOp::getOperationName(), 1,
                          context),
        variables(vars) {}

  LogicalResult
  matchAndRewrite(Operation *op, ArrayRef<Value> operands,
                  ConversionPatternRewriter &rewriter) const override;
};
} // namespace qllvm