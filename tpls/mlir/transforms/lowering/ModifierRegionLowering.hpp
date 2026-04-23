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
#include "quantum_to_llvm.hpp"

namespace qllvm {
class PowURegionOpLowering : public ConversionPattern {
public:
  explicit PowURegionOpLowering(MLIRContext *context)
      : ConversionPattern(mlir::quantum::PowURegion::getOperationName(), 1,
                          context) {}

  LogicalResult
  matchAndRewrite(Operation *op, ArrayRef<Value> operands,
                  ConversionPatternRewriter &rewriter) const override;
};

class CtrlURegionOpLowering : public ConversionPattern {
public:
  explicit CtrlURegionOpLowering(MLIRContext *context)
      : ConversionPattern(mlir::quantum::CtrlURegion::getOperationName(), 1,
                          context) {}

  LogicalResult
  matchAndRewrite(Operation *op, ArrayRef<Value> operands,
                  ConversionPatternRewriter &rewriter) const override;
};

class AdjURegionOpLowering : public ConversionPattern {
public:
  explicit AdjURegionOpLowering(MLIRContext *context)
      : ConversionPattern(mlir::quantum::AdjURegion::getOperationName(), 1,
                          context) {}

  LogicalResult
  matchAndRewrite(Operation *op, ArrayRef<Value> operands,
                  ConversionPatternRewriter &rewriter) const override;
};

class EndModifierRegionOpLowering : public ConversionPattern {
public:
  explicit EndModifierRegionOpLowering(MLIRContext *context)
      : ConversionPattern(mlir::quantum::ModifierEndOp::getOperationName(), 1,
                          context) {}

  LogicalResult
  matchAndRewrite(Operation *op, ArrayRef<Value> operands,
                  ConversionPatternRewriter &rewriter) const override;
};

struct ModifierRegionRewritePass
    : public PassWrapper<ModifierRegionRewritePass, OperationPass<ModuleOp>> {
  void getDependentDialects(DialectRegistry &registry) const override;
  void runOnOperation() final;
  ModifierRegionRewritePass() {}
};
} // namespace qllvm