#include "InputStatePreambleOptimization.hpp"
#include "CommutativeCancellationPass.hpp"
#include "ConsolidateBlocks.hpp"
#include "utils/get_matrix.hpp"
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
#include <iomanip> 
#include <string>
#include <utility>
#include <cassert>
#include "utils/circuit.hpp"
#include "utils/op.hpp"
#include "utils/gate_matrix.hpp"
#include "utils/get_matrix.hpp"
#include "utils/kak.hpp"
#include <Eigen/Dense>
#include <eigen3/unsupported/Eigen/KroneckerProduct>
#include <unordered_map>
#include <tr1/unordered_map>
#include <stdexcept>

namespace qllvm {
void InputStatePreambleOptimization::getDependentDialects(
    DialectRegistry &registry) const {
  registry.insert<LLVM::LLVMDialect>();
}
std::unordered_map<std::string, int> qbit_seq_find;
std::unordered_map<int, std::vector<std::pair<std::string, int64_t>>> hashTable_find;

void find_block_13(mlir::quantum::ValueSemanticsInstOp &op,std::vector<std::vector<mlir::quantum::ValueSemanticsInstOp>> &block_13,std::vector<mlir::quantum::ValueSemanticsInstOp> &deadOps){
  // std::cout << "find 13" << std::endl;
  if(op.name() != "ry"){
    return;
  }

  std::vector<mlir::quantum::ValueSemanticsInstOp> block;

  mlir::quantum::ValueSemanticsInstOp next1;
  mlir::quantum::ValueSemanticsInstOp next2;
  mlir::quantum::ValueSemanticsInstOp next3;
  mlir::quantum::ValueSemanticsInstOp next4;

  
  mlir::Value return_value;
  return_value = op.getResult(0);
  if(return_value.hasOneUse()){
    auto user = *return_value.user_begin();
    if(auto next_inst = dyn_cast_or_null<mlir::quantum::ValueSemanticsInstOp>(user)) {
      if(next_inst.name() == "cx"){
        next1 = next_inst;
      }else{
        return;
      }
    }else{
      return;
    }
  }else{
    return;
  }

  auto operand = next1.getOperand(1);
  if(op.getResult(0) == next1.getOperand(1)){
    operand = next1.getOperand(0);
  }
  auto operation = operand.dyn_cast_or_null<mlir::OpResult>().getOwner();
  auto owner = dyn_cast_or_null<mlir::quantum::ValueSemanticsInstOp>(operation);
  if(owner){
    if(owner.name() != "ry"){
      return;
    }
    
    operand = owner.getOperand(0);
    operation = operand.dyn_cast_or_null<mlir::OpResult>().getOwner();
    auto owner2 = dyn_cast_or_null<mlir::quantum::ValueSemanticsInstOp>(operation);
    if(owner2){
      return;
    }
  }

  return_value = next1.getResult(1);
  if(return_value.hasOneUse()){
    auto user = *return_value.user_begin();
    if(auto next_inst = dyn_cast_or_null<mlir::quantum::ValueSemanticsInstOp>(user)) {
      if(next_inst.name() != "ry"){
        return;
      }
      next2 = next_inst;
    }else{
      return;
    }
  }else{
    return;
  }

  return_value = next2.getResult(0);
  if(return_value.hasOneUse()){
    auto user = *return_value.user_begin();
    if(auto next_inst = dyn_cast_or_null<mlir::quantum::ValueSemanticsInstOp>(user)) {
      if(next_inst.name() != "cx"){
        return;
      }
  
      if(next_inst.getOperand(0) == next1.getResult(0) && next_inst.getOperand(1) == next2.getResult(0)){
        next3 = next_inst;
      }else{
        return;
      }
    }else{
      return;
    }
  }else{
    return;
  }

  mlir::OpBuilder rewriter(op);
  op.setAttr(llvm::StringRef("selected"),mlir::IntegerAttr::get(rewriter.getI32Type(), 1));
  mlir::OpBuilder rewriter3(owner);
  owner.setAttr(llvm::StringRef("selected"),mlir::IntegerAttr::get(rewriter3.getI32Type(), 1));
  mlir::OpBuilder rewriter1(next1);
  next1.setAttr(llvm::StringRef("selected"),mlir::IntegerAttr::get(rewriter1.getI32Type(), 1));
  mlir::OpBuilder rewriter2(next2);
  next2.setAttr(llvm::StringRef("selected"),mlir::IntegerAttr::get(rewriter2.getI32Type(), 1));
  mlir::OpBuilder rewriter4(next3);
  next3.setAttr(llvm::StringRef("selected"),mlir::IntegerAttr::get(rewriter4.getI32Type(), 1));

  
  block.emplace_back(op);
  block.emplace_back(owner);
  block.emplace_back(next1);
  block.emplace_back(next2);
  block.emplace_back(next3);

  block_13.emplace_back(block);

  deadOps.emplace_back(op);
  deadOps.emplace_back(owner);
  deadOps.emplace_back(next1);
  deadOps.emplace_back(next2);
  deadOps.emplace_back(next3);
}

void find_block_cx_s0(mlir::quantum::ValueSemanticsInstOp &op,std::vector<mlir::quantum::ValueSemanticsInstOp> &current_block,
                        std::vector<mlir::quantum::ValueSemanticsInstOp> &deadOps){

  auto operand = op.getOperand(0);
  auto operation = operand.dyn_cast_or_null<mlir::OpResult>().getOwner();
  auto owner = dyn_cast_or_null<mlir::quantum::ValueSemanticsInstOp>(operation);
  if(!owner){
    mlir::OpBuilder rewriter(op);
    op.setAttr(llvm::StringRef("selected"),mlir::IntegerAttr::get(rewriter.getI32Type(), 1));
    current_block.emplace_back(op);
    deadOps.emplace_back(op);
  }else{
    return;
  }
  mlir::Value return_value;
  mlir::quantum::ValueSemanticsInstOp op_temp;
  op_temp = op;
  
  while(true){
    return_value = op_temp.getResult(0);
    if(return_value.hasOneUse()){
      auto user = *return_value.user_begin();
      if(auto next_inst = dyn_cast_or_null<mlir::quantum::ValueSemanticsInstOp>(user)) {
        if(next_inst.name() == "cx"){
          if(next_inst.getOperand(0) == op_temp.getResult(0) && next_inst.getOperand(1) == op_temp.getResult(1)){
            current_block.emplace_back(next_inst);
            deadOps.emplace_back(next_inst);
            op_temp = next_inst;
          }else{
            break;
          }
        }else{
          break;
        }
      }else{
        break;
      }
    }else{
      break;
    }
  }

  operand = op.getOperand(1);
  operation = operand.dyn_cast_or_null<mlir::OpResult>().getOwner();
  owner = dyn_cast_or_null<mlir::quantum::ValueSemanticsInstOp>(operation);
 
  if(!owner){
    op_temp = current_block.back();
    while(true){
      return_value = op_temp.getResult(0);
      if(return_value.hasOneUse()){
        auto user = *return_value.user_begin();
        if(auto next_inst = dyn_cast_or_null<mlir::quantum::ValueSemanticsInstOp>(user)) {
          if(next_inst.name() == "cx"){
            if((next_inst.getOperand(0) == op_temp.getResult(0) && next_inst.getOperand(1) == op_temp.getResult(1))|| (next_inst.getOperand(0) == op_temp.getResult(1) && next_inst.getOperand(1) == op_temp.getResult(0))){
              current_block.emplace_back(next_inst);
              deadOps.emplace_back(next_inst);
              op_temp = next_inst;
            }else{
              break;
            }
          }else{
            break;
          }
        }else{
          break;
        }
      }else{
        break;
      }
    }
  }
  return;
}

void replace_block_13(std::vector<mlir::quantum::ValueSemanticsInstOp> &current_block){
  if(current_block.size() != 5){
    return;
  }

  auto ry_1 = current_block[0];
  auto ry_2 = current_block[1];
  auto cx_1 = current_block[2];
  auto ry_3 = current_block[3];
  auto cx_2 = current_block[4];

  mlir::Value inputQubit0;
  mlir::Value inputQubit1;
  mlir::Value angle_1;
  mlir::Value angle_2;
  
  if(ry_1.getResult(0) == cx_1.getOperand(0)){
    inputQubit0 = ry_1.getOperand(0);
    inputQubit1 = ry_2.getOperand(0);
    angle_1 = ry_1.getOperand(1);
    angle_2 = ry_2.getOperand(1);
  }else{
    inputQubit0 = ry_2.getOperand(0);
    inputQubit1 = ry_1.getOperand(0);
    angle_1 = ry_2.getOperand(1);
    angle_2 = ry_1.getOperand(1);
  }
  auto angle_3 = ry_3.getOperand(1);

  double angle_1_d = qllvm::OP::tryGetConstAngle(angle_1);
  double angle_2_d = qllvm::OP::tryGetConstAngle(angle_2);
  double angle_3_d = qllvm::OP::tryGetConstAngle(angle_3);

  mlir::Type qubit_type = inputQubit0.getType();
  mlir::Value angle_2_new;
  mlir::Value angle_3_new;
  mlir::Value angle_1_new;
  mlir::OpBuilder rewriter(cx_2);
  rewriter.setInsertionPointAfter(cx_2);

  auto number1 = cx_1.getAttrOfType<mlir::IntegerAttr>(llvm::StringRef("number")).getInt();
  auto index_1 = hashTable_find[number1];
  
  angle_3_new = rewriter.create<mlir::ConstantOp>(
                      cx_2.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), angle_2_d - M_PI_2));
  angle_2_new = rewriter.create<mlir::ConstantOp>(
                      cx_2.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), angle_3_d +  M_PI_2));
  angle_1_new = rewriter.create<mlir::ConstantOp>(
                      cx_2.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), angle_1_d));
  
  
  auto name = "ry";
  auto new_inst = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      cx_2->getLoc(), llvm::makeArrayRef({qubit_type}),
                      name, llvm::makeArrayRef({inputQubit0}),
                      llvm::makeArrayRef({angle_1}));
  inputQubit0 = new_inst.getResult(0);

  auto new_inst1 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      cx_2->getLoc(), llvm::makeArrayRef({qubit_type}),
                      name, llvm::makeArrayRef({inputQubit1}),
                      llvm::makeArrayRef({angle_2_new}));
  inputQubit1 = new_inst1.getResult(0);

  auto new_inst2 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      cx_2->getLoc(), llvm::makeArrayRef({qubit_type,qubit_type}),
                      "cx", llvm::makeArrayRef({inputQubit0,inputQubit1}),
                      llvm::None);
  inputQubit0 = new_inst2.getResult(0);
  inputQubit1 = new_inst2.getResult(1);

  auto new_inst3 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      cx_2->getLoc(), llvm::makeArrayRef({qubit_type}),
                      name, llvm::makeArrayRef({inputQubit1}),
                      llvm::makeArrayRef({angle_3_new}));
  inputQubit1 = new_inst3.getResult(0); 

  cx_2->getResult(0).replaceAllUsesWith(inputQubit0);
  cx_2->getResult(1).replaceAllUsesWith(inputQubit1);

}


void InputStatePreambleOptimization::runOnOperation() {

  std::vector<std::vector<mlir::quantum::ValueSemanticsInstOp>> block_13;
  std::vector<mlir::quantum::ValueSemanticsInstOp> block_17;

  std::vector<mlir::quantum::ValueSemanticsInstOp> deadops;
  std::vector<std::pair<std::string, int64_t>> idex_two;
  std::pair<std::string, int64_t> idex_one;
  int counts =0;

  getOperation().walk([&](mlir::quantum::QallocOp op) {
    qbit_seq_find.emplace(op.name().str(),counts++);
  });

  counts = 0;
  getOperation().walk([&](mlir::quantum::ValueSemanticsInstOp op) {
    idex_two.clear();
    mlir::OpBuilder rewriter(op);
    op.setAttr(llvm::StringRef("number"),mlir::IntegerAttr::get(rewriter.getI32Type(), counts));
    op.setAttr(llvm::StringRef("selected"),mlir::IntegerAttr::get(rewriter.getI32Type(), 0));
    op.setAttr(llvm::StringRef("onebit"),mlir::IntegerAttr::get(rewriter.getI32Type(), 0));

    if(op.getNumResults() == 1){
      idex_one = OP::getbit_from_valueSemanticsInstOp(op);
      idex_two.emplace_back(idex_one);
    }else if(op.getNumResults() == 2){
      idex_two = OP::getbit_from_muti_valueSemanticsInstOp(op);
    }
    hashTable_find[counts] = idex_two;
    counts++;
  });

  topop.clear();
  circuit::getGateCountAndTopOp(before_gate_count, topop, getOperation());

  int rz_pi = 0;
  for(auto &op : topop){
    if(op.name() == "rz"){

      rz_pi++;
      op->getResult(0).replaceAllUsesWith(op.getOperand(0));
      op->dropAllUses();
      op.erase();
    }
  }

  //  update top_op
  topop.clear();
  circuit::getGateCountAndTopOp(before_gate_count, topop, getOperation());
  int ry_pi = 0;
  std::vector<mlir::quantum::ValueSemanticsInstOp> cx_ops;
  for(auto &op : topop){
    if(op.name() == "ry"){
      if(qllvm::OP::tryGetConstAngle(op.getOperand(1)) == M_PI){
        ry_pi++;
        cx_ops.clear();
        // std::cout << "qllvm::OP::tryGetConstAngle(op.getOperand(1)): " <<qllvm::OP::tryGetConstAngle(op.getOperand(1)) << std::endl;
        auto return_value = op.getOperand(0);
        while(true){
          if(return_value.hasOneUse()){
            auto user = *return_value.user_begin();
            if(auto next_inst = dyn_cast_or_null<mlir::quantum::ValueSemanticsInstOp>(user)) {
              if(next_inst.name() == "cx"){
                ry_pi++;
                cx_ops.emplace_back(next_inst);
                return_value = next_inst.getOperand(0);
              }else{
                break;
              }
            }else{
              break;
            }
          }else{
            break;
          }
        }
        for(auto &elem: cx_ops){
          auto inputQubit = elem.getOperand(1);
          mlir::Type qubit_type = inputQubit.getType();
          mlir::Value angle;

          mlir::OpBuilder rewriter(elem);
          rewriter.setInsertionPointAfter(elem);

          angle = rewriter.create<mlir::ConstantOp>(
                              elem.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), M_PI));
          
          auto new_inst = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                              elem->getLoc(), llvm::makeArrayRef({qubit_type}),
                              "rz", llvm::makeArrayRef({inputQubit}),
                              llvm::makeArrayRef({angle}));
          inputQubit = new_inst.getResult(0);
          auto new_inst2 = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                              elem->getLoc(), llvm::makeArrayRef({qubit_type}),
                              "ry", llvm::makeArrayRef({inputQubit}),
                              llvm::makeArrayRef({angle}));
          elem->getResult(0).replaceAllUsesWith(elem.getOperand(0));
          elem->dropAllUses();
          elem.erase();
        }
        
        // op->getResult(0).replaceAllUsesWith(op.getOperand(0));
        // op->dropAllUses();
        // op.erase();
      }
    }
  }
  // std::cout << "rz_pi start " << rz_pi << std::endl;
  // std::cout << "ry_pi start " << ry_pi << std::endl;
  // return;

  //  update top_op
  topop.clear();
  circuit::getGateCountAndTopOp(before_gate_count, topop, getOperation());

  int cx_0_count = 0;
  int delate_cx_count = 0;
  while(true){
    auto cx_0_count_init = cx_0_count;
    for(auto &op : topop){
      deadops.clear();
      if(op.name() == "cx"){
        block_17.clear();
        find_block_cx_s0(op,block_17,deadops);
        if(block_17.size() > 0){
          auto last_op = block_17.back();
          auto number1 = op.getAttrOfType<mlir::IntegerAttr>(llvm::StringRef("number")).getInt();
          auto temp1 = hashTable_find[number1];

          auto number2 = last_op.getAttrOfType<mlir::IntegerAttr>(llvm::StringRef("number")).getInt();
          auto temp2 = hashTable_find[number2];

          if(temp1 == temp2){
            last_op->getResult(0).replaceAllUsesWith(op.getOperand(0));
            last_op->getResult(1).replaceAllUsesWith(op.getOperand(1));
          }else{
            last_op->getResult(0).replaceAllUsesWith(op.getOperand(1));
            last_op->getResult(1).replaceAllUsesWith(op.getOperand(0));
          }
          delate_cx_count += deadops.size();
          cx_0_count_init ++;

          for(auto &elem: deadops){
            elem->dropAllUses();
            elem.erase();
          }
        }
      }
    }
    topop.clear();
    circuit::getGateCountAndTopOp(before_gate_count, topop, getOperation());
    if(cx_0_count_init == cx_0_count){
      break;
    }else{
      cx_0_count = cx_0_count_init;
    }
  }
  // std::cout << "cx  before bit control there is no gate " << cx_0_count << "all delete gates is "<<  delate_cx_count << std::endl;
  
  deadops.clear();
  // auto module_size = merge_single_gate_module(getOperation());

  // auto module = module_size.first;
  // auto size = module_size.second;

  getOperation().walk([&](mlir::quantum::ValueSemanticsInstOp op) {
    mlir::OpBuilder rewriter(op);
    op.setAttr(llvm::StringRef("selected"),mlir::IntegerAttr::get(rewriter.getI32Type(), 0));
    op.setAttr(llvm::StringRef("onebit"),mlir::IntegerAttr::get(rewriter.getI32Type(), 0));
  });

  topop.clear();
  circuit::getGateCountAndTopOp(before_gate_count, topop, getOperation());

  for(auto &op : topop){
    int selected = op.getAttrOfType<mlir::IntegerAttr>(llvm::StringRef("selected")).getInt();
    if(op.getNumResults() == 1 && selected == 0){
      find_block_13(op,block_13,deadops);
    }
  }
  for(auto &elem: block_13){
    replace_block_13(elem);
  }
  for(auto &op: deadops){
    op->dropAllUses();
    op.erase();
  }

  deadops.clear();

  topop.clear();
  circuit::getGateCountAndTopOp(before_gate_count, topop, getOperation());
  for(auto &op : topop){
    if(op.name() == "rz"){
      // rz_pi++;
      op->getResult(0).replaceAllUsesWith(op.getOperand(0));
      op->dropAllUses();
      op.erase();
    }
  }

}
} // namespace qllvm
