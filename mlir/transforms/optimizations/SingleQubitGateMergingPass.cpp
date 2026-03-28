#include "SingleQubitGateMergingPass.hpp"
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

namespace qllvm {
void SingleQubitGateMergingPass::getDependentDialects(
    DialectRegistry &registry) const {
  registry.insert<LLVM::LLVMDialect>();
}
std::unordered_set<std::string> basicgateset;

void SingleQubitGateMergingPass::runOnOperation() {

  basicgateset = basic_gate;

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
  // Walk the operations within the function.
  std::vector<mlir::quantum::ValueSemanticsInstOp> deadOps;
  std::vector<std::string> unableOps = {"mz","reset","u1","u2","p","u"};
  getOperation().walk([&](mlir::quantum::ValueSemanticsInstOp op) {
    auto inst_name = op.name();
    if (std::find(deadOps.begin(), deadOps.end(), op) != deadOps.end()) {
      // Skip this op since it was merged (forward search)
      return;
    }
    mlir::OpBuilder rewriter(op);

    // List of ops:
    std::vector<mlir::quantum::ValueSemanticsInstOp> ops_list;
    std::vector<std::vector<double>> op_params;
    auto current_op = op;

    // Helper to retrieve VSOp constant params:
    const auto retrieveConstantGateParams =
        [](mlir::quantum::ValueSemanticsInstOp &vs_op) -> std::vector<double> {
          
      if (vs_op.getNumOperands() > 1) {
        // that there are rotation phase parameters
        // Parameterized gate:
        std::vector<double> current_op_params;
        for (size_t i = 1; i < vs_op.getNumOperands(); ++i) {
          auto param = vs_op.getOperand(i);
          assert(param.getType().isa<mlir::FloatType>());
          double param_val = qllvm::OP::tryGetConstAngle(param);
          current_op_params.emplace_back(param_val);

        }
        return current_op_params;
      } else {
        return {};
      }
    };
    for (;;) {
      // Break inside:
      if (current_op.qubits().size() > 1) {
        break;
      }
      auto return_value = *current_op.result().begin();
      if(std::find(unableOps.begin(),unableOps.end(),current_op.name().str())!=unableOps.end()){
        break;
      }
      if (return_value.hasOneUse()) {
        const auto const_op_params = retrieveConstantGateParams(current_op);
        ops_list.emplace_back(current_op);
        op_params.emplace_back(const_op_params);
        // get that one user
        auto user = *return_value.user_begin();
        if (auto next_inst =
                dyn_cast_or_null<mlir::quantum::ValueSemanticsInstOp>(user)) {
          // Only allow unitary gates...
          if(std::find(unableOps.begin(),unableOps.end(),current_op.name().str())!=unableOps.end()){
            break;
          } else {
            // Continue the search
            current_op = next_inst;
          }
        } else {
          break;
        }
      } else {
        if (current_op.qubits().size() == 1) {
          const auto const_op_params = retrieveConstantGateParams(current_op);

          ops_list.emplace_back(current_op);
          op_params.emplace_back(const_op_params);
        }
        break;
      }
    }

    assert(ops_list.size() == op_params.size());
    constexpr int MIN_SEQ_LENGTH = 2;
    if (ops_list.size() >= MIN_SEQ_LENGTH) {
      // Should try to optimize:
      std::vector<qllvm::utils::qop_t> ops;
      for (size_t i = 0; i < ops_list.size(); ++i) {
        ops.emplace_back(
            std::make_pair(ops_list[i].name().str(), op_params[i]));
      }
      const auto simplified_seq = qllvm::utils::new_euler_decompose(ops,basicgateset);
      
      if (simplified_seq.size() < ops_list.size()) {
        rewriter.setInsertionPointAfter(ops_list.back());
        std::vector<mlir::quantum::ValueSemanticsInstOp> new_ops;
        for (const auto &[pauli_inst, thetas] : simplified_seq) {
          std::vector<mlir::Value> params;
          for(auto theta : thetas){
            mlir::Value theta_val = rewriter.create<mlir::ConstantOp>(
              op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), theta));
            params.emplace_back(theta_val);
          }
          if(params.size()!=0){
            std::vector<mlir::Type> ret_types{op.getOperand(0).getType()};
            auto new_inst = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                op.getLoc(), llvm::makeArrayRef(ret_types), pauli_inst,
                llvm::makeArrayRef(new_ops.empty() ? op.getOperand(0) : *(new_ops.back().result_begin())),
                llvm::makeArrayRef(params));
            new_ops.emplace_back(new_inst);
          }else{
            std::vector<mlir::Type> ret_types{op.getOperand(0).getType()};
            auto new_inst = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                op.getLoc(), llvm::makeArrayRef(ret_types), pauli_inst,
                llvm::makeArrayRef(new_ops.empty() ? op.getOperand(0) : *(new_ops.back().result_begin())),
                llvm::None);
            new_ops.emplace_back(new_inst);
          }
        }

        // Input -> Output mapping (this instruction is to be removed)
        auto last_inst_orig = ops_list.back();
        if (new_ops.empty()) {
          auto first_inst_orig = ops_list.front();
          (*last_inst_orig.result_begin())
              .replaceAllUsesWith(first_inst_orig.getOperand(0));
        } else {
          auto last_inst_new = new_ops.back();
          (*last_inst_orig.result_begin())
              .replaceAllUsesWith(*last_inst_new.result_begin());
        }

        // Erase original instructions:
        for (auto &op_to_delete : ops_list) {
          deadOps.emplace_back(op_to_delete);
        }
        
      }
    }
  });

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

