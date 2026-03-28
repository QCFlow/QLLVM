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