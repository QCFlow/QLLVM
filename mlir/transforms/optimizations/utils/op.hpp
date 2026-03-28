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
