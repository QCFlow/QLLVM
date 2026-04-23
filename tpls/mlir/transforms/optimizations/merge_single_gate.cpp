#include "merge_single_gate.hpp"
#include "gen_qasm.hpp"

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
#include <iostream>
#include <cmath>
#include "utils/op.hpp"
bool is_c(double a, double b) {
    // 计算角度差
    if(std::abs(a-b) < 1.0e-16){
        return true;
    }else{
        return false;
    }
}

namespace qllvm {
void merge_single_gate::getDependentDialects(DialectRegistry &registry) const {
  registry.insert<LLVM::LLVMDialect>();
}
int new_gate_count = 0;

void find_rz_block(std::vector<mlir::quantum::ValueSemanticsInstOp> &current_run,mlir::quantum::ValueSemanticsInstOp op){
  mlir::quantum::ValueSemanticsInstOp op_t;
  op_t = op;
  while(true){
    auto return_value = *op_t.result().begin();
    if (return_value.hasOneUse()) {
      auto user = *return_value.user_begin();
      if (auto next_inst = dyn_cast_or_null<mlir::quantum::ValueSemanticsInstOp>(user)) {
        if(next_inst.getNumResults() == 1){    
          int onebit = next_inst.getAttrOfType<mlir::IntegerAttr>(llvm::StringRef("selected")).getInt();
          if(onebit == 0 && next_inst.name() == "rz"){
            mlir::OpBuilder rewriter(next_inst);
            next_inst.setAttr(llvm::StringRef("selected"),mlir::IntegerAttr::get(rewriter.getI32Type(), 1));
            current_run.emplace_back(next_inst);
            op_t = next_inst;
          }else{
            return;
          }
        }else{
          return;
        }
      }else{
        return;
      }
    }else{
      return;
    }
  }
}

void find_ry_block(std::vector<mlir::quantum::ValueSemanticsInstOp> &current_run,mlir::quantum::ValueSemanticsInstOp op){
  mlir::quantum::ValueSemanticsInstOp op_t;
  op_t = op;
  while(true){
    auto return_value = *op_t.result().begin();
    if (return_value.hasOneUse()) {
      auto user = *return_value.user_begin();
      if (auto next_inst = dyn_cast_or_null<mlir::quantum::ValueSemanticsInstOp>(user)) {
        if(next_inst.getNumResults() == 1){    
          int onebit = next_inst.getAttrOfType<mlir::IntegerAttr>(llvm::StringRef("selected")).getInt();
          if(onebit == 0 && next_inst.name() == "ry"){
            mlir::OpBuilder rewriter(next_inst);
            next_inst.setAttr(llvm::StringRef("selected"),mlir::IntegerAttr::get(rewriter.getI32Type(), 1));
            current_run.emplace_back(next_inst);
            op_t = next_inst;
          }else{
            return;
          }
        }else{
          return;
        }
      }else{
        return;
      }
    }else{
      return;
    }
  }
}

void deal_block(std::vector<mlir::quantum::ValueSemanticsInstOp> &current_run,std::string name,std::vector<mlir::quantum::ValueSemanticsInstOp> &dead_ops){
  double total_theta = 0.0;
  for(auto &elem: current_run){
    total_theta += qllvm::OP::tryGetConstAngle(elem.getOperand(1));
    dead_ops.emplace_back(elem);
  }

  double result = std::fmod(total_theta,2*M_PI);
  
  auto last = current_run.back();
  auto first = current_run.front();

  if(is_c(result,0)){
    last->getResult(0).replaceAllUsesWith(first.getOperand(0));
    return;
  }

  mlir::OpBuilder rewriter(last);
  rewriter.setInsertionPointAfter(last);
  
  mlir::Value inputQubit = first.getOperand(0);
  mlir::Type qubit_type = inputQubit.getType();

  mlir::Value theta = rewriter.create<mlir::ConstantOp>(
                    last.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), result));

  auto new_inst = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      last.getLoc(), llvm::makeArrayRef({qubit_type}),
                      name, llvm::makeArrayRef({inputQubit}),
                      llvm::makeArrayRef({theta}));
  last->getResult(0).replaceAllUsesWith(new_inst.getResult(0));
  new_gate_count += 1;
}

std::pair<mlir::ModuleOp, int64_t> merge_single_gate_module(mlir::ModuleOp module_ops) {
  // std::cout << "merge_singe_gate" << std::endl;
  module_ops.walk([&](mlir::quantum::ValueSemanticsInstOp op) {
    mlir::OpBuilder rewriter(op);
    op.setAttr(llvm::StringRef("selected"),mlir::IntegerAttr::get(rewriter.getI32Type(), 0));
  });

  std::vector<mlir::quantum::ValueSemanticsInstOp> deadOps;
  std::vector<std::vector<mlir::quantum::ValueSemanticsInstOp>> ry_blocks;
  std::vector<std::vector<mlir::quantum::ValueSemanticsInstOp>> rz_blocks;
  std::vector<mlir::quantum::ValueSemanticsInstOp> current_run;

  module_ops.walk([&](mlir::quantum::ValueSemanticsInstOp op) {
    auto inst_name = op.name();
    int selected = op.getAttrOfType<mlir::IntegerAttr>(llvm::StringRef("selected")).getInt();
    if(op.name() == "rz" && selected == 0){
      mlir::OpBuilder rewriter(op);
      op.setAttr(llvm::StringRef("selected"),mlir::IntegerAttr::get(rewriter.getI32Type(), 1));
      current_run.clear();
      current_run.emplace_back(op);
      find_rz_block(current_run,op);
      if(current_run.size() > 1){
        rz_blocks.emplace_back(current_run);
      }else if(current_run.size() == 1){
        if(qllvm::OP::tryGetConstAngle(op.getOperand(1)) == 0.0000){
          op->getResult(0).replaceAllUsesWith(op.getOperand(0));
          deadOps.emplace_back(op);
        }
      }
    }else if(op.name() == "ry" && selected == 0){
      mlir::OpBuilder rewriter(op);
      op.setAttr(llvm::StringRef("selected"),mlir::IntegerAttr::get(rewriter.getI32Type(), 1));
      current_run.clear();
      current_run.emplace_back(op);
      find_ry_block(current_run,op);
      if(current_run.size() > 1){
        ry_blocks.emplace_back(current_run);
      }else if(current_run.size() == 1){
        if(qllvm::OP::tryGetConstAngle(op.getOperand(1)) == 0.0000){
          op->getResult(0).replaceAllUsesWith(op.getOperand(0));
          deadOps.emplace_back(op);
        }
      }
    }
  });

  for(auto &elem: rz_blocks){
    deal_block(elem,"rz",deadOps);
  }
  for(auto &elem: ry_blocks){
    deal_block(elem,"ry",deadOps);
  }
  int all_size = deadOps.size();
  for (auto &op : deadOps) {
    op->dropAllUses();
    op.erase();
  }
  // std::cout << "after deleted cx created one bit size: " << new_gate_count << std::endl;
  return std::make_pair(module_ops,all_size);

}


void merge_single_gate::runOnOperation() {
  getOperation().walk([&](mlir::quantum::ValueSemanticsInstOp op) {
    mlir::OpBuilder rewriter(op);
    op.setAttr(llvm::StringRef("selected"),mlir::IntegerAttr::get(rewriter.getI32Type(), 0));
  });

  std::vector<mlir::quantum::ValueSemanticsInstOp> deadOps;
  std::vector<std::vector<mlir::quantum::ValueSemanticsInstOp>> ry_blocks;
  std::vector<std::vector<mlir::quantum::ValueSemanticsInstOp>> rz_blocks;
  std::vector<mlir::quantum::ValueSemanticsInstOp> current_run;

  getOperation().walk([&](mlir::quantum::ValueSemanticsInstOp op) {
    auto inst_name = op.name();
    int selected = op.getAttrOfType<mlir::IntegerAttr>(llvm::StringRef("selected")).getInt();
    if(op.name() == "rz" && selected == 0){
      mlir::OpBuilder rewriter(op);
      op.setAttr(llvm::StringRef("selected"),mlir::IntegerAttr::get(rewriter.getI32Type(), 1));
      current_run.clear();
      current_run.emplace_back(op);
      find_rz_block(current_run,op);
      if(current_run.size() > 1){
        rz_blocks.emplace_back(current_run);
      }else if(current_run.size() == 1){
        if(qllvm::OP::tryGetConstAngle(op.getOperand(1)) == 0.0000){
          op->getResult(0).replaceAllUsesWith(op.getOperand(0));
          deadOps.emplace_back(op);
        }
      }
    }else if(op.name() == "ry" && selected == 0){
      mlir::OpBuilder rewriter(op);
      op.setAttr(llvm::StringRef("selected"),mlir::IntegerAttr::get(rewriter.getI32Type(), 1));
      current_run.clear();
      current_run.emplace_back(op);
      find_ry_block(current_run,op);
      if(current_run.size() > 1){
        ry_blocks.emplace_back(current_run);
      }else if(current_run.size() == 1){
        if(qllvm::OP::tryGetConstAngle(op.getOperand(1)) == 0.0000){
          op->getResult(0).replaceAllUsesWith(op.getOperand(0));
          deadOps.emplace_back(op);
        }
      }
    }
  });

  for(auto &elem: rz_blocks){
    deal_block(elem,"rz",deadOps);
  }
  for(auto &elem: ry_blocks){
    deal_block(elem,"ry",deadOps);
  }
  int all_size = deadOps.size();
  for (auto &op : deadOps) {
    op->dropAllUses();
    op.erase();
  }
}



}




