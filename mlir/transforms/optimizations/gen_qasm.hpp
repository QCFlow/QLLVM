#include "Quantum/QuantumOps.h"
#include "mlir/Pass/Pass.h"
#include "mlir/Pass/PassManager.h"
#include "mlir/Target/LLVMIR.h"
#include "mlir/Transforms/DialectConversion.h"
#include "mlir/Transforms/Passes.h"
#include <unordered_map>
#include <tr1/unordered_map>
#include <iostream>

using namespace mlir;

namespace qllvm {

  // void gen_qasm(mlir::ModuleOp module_ops);
  void gen_qasm(mlir::ModuleOp module_ops,std::string qasmName = "compiled.qasm");

} // namespace qllvm
