#include "Merge_u3_gate.hpp"
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
#include "utils/gate_matrix.hpp"
#include <iostream>
#include <iomanip> 
#include "utils/circuit.hpp"
#include "utils/op.hpp"
#include "ConsolidateBlocks.hpp"

namespace qllvm {
void Merge_u3_gate::getDependentDialects(
    DialectRegistry &registry) const {
  registry.insert<LLVM::LLVMDialect>();
}

void merge_us_list(std::vector<mlir::quantum::ValueSemanticsInstOp> &run,std::vector<mlir::quantum::ValueSemanticsInstOp> &deadOps){
  for(auto &op : run){
    deadOps.emplace_back(op);
  }
  std::vector<mlir::Value> params_values;
  auto mat = qllvm::totalMat(run);
  auto params = qllvm::utils::paramszyzinner(mat);
  if (!params.empty()) {   // prevent undefined behavior caused by calling pop_back on an empty vector
    params.pop_back();
  }
  auto last = run.back();
  auto first = run.front();

  mlir::OpBuilder rewriter(last);
  rewriter.setInsertionPointAfter(last);
  
  for(auto theta: params){
    auto theta_val = rewriter.create<mlir::ConstantOp>(
                          last.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), theta));
    params_values.emplace_back(theta_val);
  }
  
  mlir::Value inputQubit = first.getOperand(0);
  mlir::Type qubit_type = inputQubit.getType();

  auto new_inst = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      last.getLoc(), llvm::makeArrayRef({qubit_type}),
                      "u3", llvm::makeArrayRef({inputQubit}),
                      llvm::makeArrayRef(params_values));

  last.getResult(0).replaceAllUsesWith(*new_inst.result_begin());
}

void Merge_u3_gate::runOnOperation() {
  if(syn&&*e == 0)
    return;
  if (f == true)
    *c+=1;
  if(printCountAndDepth||syn){
    circuit::getGateCountAndTopOp(before_gate_count, top_op, getOperation());
    int depth = 0;
    if (*c_d==0){
      for (auto &op : top_op) {
        depth = circuit::getDepth(op);
        before_circuit_depth = depth > before_circuit_depth ? depth : before_circuit_depth;
      }
    }else{
      before_circuit_depth = *c_d;
    }
  }
  
  getOperation().walk([&](mlir::quantum::ValueSemanticsInstOp op) {
    mlir::OpBuilder rewriter(op);
    op.getOperation()->setAttr(llvm::StringRef("onebit"),mlir::IntegerAttr::get(rewriter.getI32Type(), 0));
  });

  // Walk the operations within the function.
  std::vector<mlir::quantum::ValueSemanticsInstOp> deadOps;
  std::vector<std::vector<mlir::quantum::ValueSemanticsInstOp>> runs;
  getOperation().walk([&](mlir::quantum::ValueSemanticsInstOp op) {
    int onebit = op.getOperation()->getAttrOfType<mlir::IntegerAttr>(llvm::StringRef("onebit")).getInt();
    std::vector<mlir::quantum::ValueSemanticsInstOp> current_run;
    if(onebit == 0 && op.getNumResults() == 1){
      current_run.emplace_back(op);
      find_runs(current_run,op,1);
      runs.emplace_back(current_run);
    }
  });

  deadOps.clear();
  for(auto &run : runs){
      merge_us_list(run,deadOps);
  }

  for (auto &op : deadOps) {
    op->dropAllUses();
    op.erase();
  }

  
  if(printCountAndDepth||syn){
    circuit::getGateCountAndTopOp(after_gate_count, top_op, getOperation());
    int depth = 0;
    for (auto &op : top_op) {
      depth = circuit::getDepth(op);
      after_circuit_depth = depth > after_circuit_depth ? depth : after_circuit_depth;
      *c_d = after_circuit_depth;
    }
    if(printCountAndDepth){
      *p += (before_gate_count-after_gate_count);
      *q += (before_circuit_depth-after_circuit_depth);
    }
    if(syn){
      if((before_gate_count-after_gate_count) == 0 && (before_circuit_depth-after_circuit_depth) == 0){
        *o += 1;
        if(*o == 5)
          *e = 0;
        else
          *e = 1;
      }else{
        *o = 0;
      }
    }
  }
}
} // namespace qllvm

