#pragma once
#include <string>
#include <utility>
#include <vector>
#include <Eigen/Dense>
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
namespace matrix {
    using qop_t = std::pair<std::string, std::vector<double>>;
    Eigen::MatrixXcd getGateMat(mlir::quantum::ValueSemanticsInstOp &op, std::unordered_map<std::string, int> qbitseq);
    Eigen::MatrixXcd getSingleGateMat(const qop_t &in_op);
    Eigen::MatrixXcd getGateMat(const qop_t &in_op,int ForwardOrReverse);
    bool ForwardOrReverse(std::vector<std::pair<std::string, int64_t>> bits,std::unordered_map<std::string, int> qbitseq);
    Eigen::MatrixXcd getGateMat(mlir::quantum::ValueSemanticsInstOp &op,int i);
  
} // namespace utils
} // namespace qllvm