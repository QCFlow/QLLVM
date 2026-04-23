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
#include "ConsolidateBlocks.hpp"
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
#include <string>
#include <utility>
#include <cassert>
#include "utils/circuit.hpp"
#include "utils/op.hpp"
#include "utils/gate_matrix.hpp"
#include "utils/get_matrix.hpp"
#include "utils/kak.hpp"
#include "CommutativeCancellationPass.hpp"
#include <Eigen/Dense>
#include <eigen3/unsupported/Eigen/KroneckerProduct>
#include <eigen3/unsupported/Eigen/MatrixFunctions>
#include <unordered_map>
#include <tr1/unordered_map>
#include <cmath>
#include <algorithm>
#include <cstdio>

namespace qllvm {
using namespace std::complex_literals;
void ConsolidateBlocks::getDependentDialects(DialectRegistry &registry) const {
  registry.insert<LLVM::LLVMDialect>();
}

std::unordered_map<std::string, int> qbit_vect;
std::unordered_map<int, std::vector<std::pair<std::string, int64_t>>> hashTableindex;
std::unordered_set<std::string> basic_gateset;

// calculate the matrix of single-qubit or two-qubit quantum gate block
Eigen::MatrixXcd totalMat(std::vector<mlir::quantum::ValueSemanticsInstOp> &blocks){
  int current_bit = 0;
  int total_bit = 0;
  bool Positive;
  std::vector<mlir::quantum::ValueSemanticsInstOp> block;
  block = blocks;

  reverse(block.begin(),block.end());

  Eigen::MatrixXcd id_op = Eigen::MatrixXcd::Identity(2, 2);
  Eigen::MatrixXcd total_mat;
  Eigen::MatrixXcd current_mat;
  Eigen::MatrixXcd temp1;
  Eigen::MatrixXcd temp2;

  std::vector<std::pair<std::string, int64_t>> total_qbits_index;
  std::vector<std::pair<std::string, int64_t>> current_qbits_index;
  std::pair<std::string, int64_t> index_1qbits;

  for(auto &op : block){
    current_bit = op.getNumResults();
    auto number = op.getOperation()->getAttrOfType<mlir::IntegerAttr>(llvm::StringRef("location")).getInt();
    current_qbits_index = hashTableindex[number];
    if(current_bit > 1){
      // current_qbits_index = OP::getbit_from_muti_valueSemanticsInstOp(op);
      Positive = qllvm::matrix::ForwardOrReverse(current_qbits_index,qbit_vect);
      if(Positive){
        current_mat = qllvm::matrix::getGateMat(op,0);
      }else{
        current_mat = qllvm::matrix::getGateMat(op,1);
      }
    }else{
      current_mat = qllvm::matrix::getGateMat(op,0);
      // index_1qbits = OP::getbit_from_valueSemanticsInstOp(op);
      index_1qbits = current_qbits_index.front();
    }

    if(op == block.front())
    {
      total_mat = current_mat;
      total_bit = current_bit;
      if(current_bit == 1){
        total_qbits_index.emplace_back(index_1qbits);
      }else{
        if(!Positive){
          total_qbits_index.emplace_back(current_qbits_index.back());
          total_qbits_index.emplace_back(current_qbits_index.front());
        }else{
          total_qbits_index = current_qbits_index;
        }
      }  
    }else{
      if(current_bit == total_bit){
        if(current_bit == 2){
          total_mat = total_mat * current_mat;
        }else if(current_bit == 1){
          total_qbits_index.emplace_back(index_1qbits);
          if(index_1qbits == total_qbits_index.front()){
            total_mat = total_mat * current_mat;
            total_qbits_index.pop_back();
          }else{
            auto if_positive = qllvm::matrix::ForwardOrReverse(total_qbits_index,qbit_vect);
            if(if_positive){
              temp1 = Eigen::kroneckerProduct(id_op,total_mat);
              temp2 = Eigen::kroneckerProduct(current_mat,id_op);
              total_mat = temp1 * temp2;
              total_bit = 2;
            }else{
              temp1 = Eigen::kroneckerProduct(total_mat,id_op);
              temp2 = Eigen::kroneckerProduct(id_op,current_mat);
              total_mat = temp1 * temp2;
              total_bit = 2;
              total_qbits_index.pop_back();
              total_qbits_index.insert(total_qbits_index.begin(),index_1qbits);
            }
          }
            
        }
      }else if(current_bit < total_bit){
        if(index_1qbits == total_qbits_index.front()){
          // temp1 = Eigen::kroneckerProduct(current_mat,id_op);
          temp1 = Eigen::kroneckerProduct(id_op,current_mat);
        }else{
          temp1 = Eigen::kroneckerProduct(current_mat,id_op);
          // temp1 = Eigen::kroneckerProduct(id_op,current_mat);
        }
        total_mat = total_mat * temp1;
      }else if(current_bit > total_bit){
        index_1qbits = total_qbits_index.front();
        total_qbits_index.clear();
        if(!Positive){
          total_qbits_index.emplace_back(current_qbits_index.back());
          total_qbits_index.emplace_back(current_qbits_index.front());
        }else{
          total_qbits_index = current_qbits_index;
        }

        if(total_qbits_index.front() == index_1qbits){
          // temp1 = Eigen::kroneckerProduct(total_mat,id_op);
          temp1 = Eigen::kroneckerProduct(id_op,total_mat);
        }else{
          // temp1 = Eigen::kroneckerProduct(id_op,total_mat);
          temp1 = Eigen::kroneckerProduct(total_mat,id_op);
        }
        total_bit = 2;
        total_mat = temp1 * current_mat;
      }
    }
  }
  return total_mat;
}

//   find the single-qubit quantum gate block
void find_runs(std::vector<mlir::quantum::ValueSemanticsInstOp> &current_run,
                mlir::quantum::ValueSemanticsInstOp op,int owner_or_user){
  //current_run: the incomplete run sequence, op: the current gate, owner_or_user=0 find the previous gate, owner_or_user=1 find the next gate
  mlir::quantum::ValueSemanticsInstOp op_t;
  op_t = op;
  while(true){
    auto operand = op_t.getOperand(0);
    auto operation = operand.dyn_cast_or_null<mlir::OpResult>().getOwner();
    auto owner = dyn_cast_or_null<mlir::quantum::ValueSemanticsInstOp>(operation);
    //user
    auto return_value = *op_t.result().begin();
    if(owner_or_user == 0){
      if(owner){
        if(owner.getNumResults() == 1){
          if(owner.name() == "mz"){
            return;
          }
          mlir::OpBuilder rewriter(owner);
          // label the onebit, represent that the single-qubit gate has been found, ensure that there is no duplicate single-qubit gate sequence
          int onebit = owner.getOperation()->getAttrOfType<mlir::IntegerAttr>(llvm::StringRef("onebit")).getInt();
          if(onebit == 0){
            current_run.insert(current_run.begin(), owner);
            owner.getOperation()->setAttr(llvm::StringRef("onebit"),mlir::IntegerAttr::get(rewriter.getI32Type(), 1));
            op_t = owner;
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
      if (return_value.hasOneUse()) {
        // get that one user
        auto user = *return_value.user_begin();
        // cast to a inst op
        if (auto next_inst = dyn_cast_or_null<mlir::quantum::ValueSemanticsInstOp>(user)) {
          if(next_inst.getNumResults() == 1){    
            mlir::OpBuilder rewriter(next_inst);
            int onebit = next_inst.getOperation()->getAttrOfType<mlir::IntegerAttr>(llvm::StringRef("onebit")).getInt();
            if(onebit == 0){
              current_run.emplace_back(next_inst);
              next_inst.getOperation()->setAttr(llvm::StringRef("onebit"),mlir::IntegerAttr::get(rewriter.getI32Type(), 1));
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
  
}

bool judge_same_index(std::vector<std::pair<std::string, int64_t>> index_block,std::vector<mlir::quantum::ValueSemanticsInstOp> &sideop,mlir::quantum::ValueSemanticsInstOp &op,int owner_user){
  /*
  index_block: the index of the block
  op: the current quantum gate
  sideop: the outermost quantum gate
  owner_user:  owner_user = 0 the leftmost, owner_user = 1 the rightmost
  */ 
  
  mlir::quantum::ValueSemanticsInstOp tmp_op;
  mlir::quantum::ValueSemanticsInstOp tmp_op2; 

  // to ensure that the gate does not appear in different blocks
  if(op.getOperation()->getAttrOfType<mlir::IntegerAttr>(llvm::StringRef("selected")).getInt()){
    return false;
  }
  // check if the index is the same
  if(op.getNumResults() == 1){
    return true;
  }

  // the current is a two-qubit quantum gate
  if(op.getNumResults() == 2){
    // get the index of the current quantum gate
    auto number = op.getOperation()->getAttrOfType<mlir::IntegerAttr>(llvm::StringRef("location")).getInt();
    auto index = hashTableindex[number];
    // find the position of the original quantum gate index in the current quantum gate index
    auto it1 = std::find(index_block.begin(), index_block.end(), index.front());
    auto it2 = std::find(index_block.begin(), index_block.end(), index.back());
    
    // the index of the current quantum gate is the same as the index of the block
    if(it1 != index_block.end() && it2 != index_block.end()){
      tmp_op = sideop.front();
      tmp_op2 = sideop.back();
      std::vector<mlir::Value> oprands;
      auto it_1 = std::find(oprands.begin(), oprands.end(), op.getResult(0));
      auto it_2 = std::find(oprands.begin(), oprands.end(), op.getResult(0));

      if(owner_user == 0){
        // get the all Oprand parameters of the leftmost quantum gate of the block (input)
        oprands.emplace_back(tmp_op.getOperand(0));
        oprands.emplace_back(tmp_op2.getOperand(0));
        if(tmp_op.getNumResults() == 2){
          oprands.emplace_back(tmp_op.getOperand(1));
        }
        if(tmp_op2.getNumResults() == 2){
          oprands.emplace_back(tmp_op2.getOperand(1));
        }
        // check if the result parameters of the current quantum gate are in the Oprand parameters
        it_1 = std::find(oprands.begin(), oprands.end(), op.getResult(0));
        it_2 = std::find(oprands.begin(), oprands.end(), op.getResult(1));
      }else if(owner_user == 1){
        // get the all Result parameters of the leftmost quantum gate of the block (output)
        oprands.emplace_back(tmp_op.getResult(0));
        oprands.emplace_back(tmp_op2.getResult(0));
        if(tmp_op.getNumResults() == 2){
          oprands.emplace_back(tmp_op.getResult(1));
        }
        if(tmp_op2.getNumResults() == 2){
          oprands.emplace_back(tmp_op2.getResult(1));
        }
        // check if the Operand parameters of the current quantum gate are in the Result parameters
        it_1 = std::find(oprands.begin(), oprands.end(), op.getOperand(0));
        it_2 = std::find(oprands.begin(), oprands.end(), op.getOperand(1));
      }
      // op has the same index as the block
      if(it_1 != oprands.end() && it_2 != oprands.end()){
        return true;
      }
    }
  }
  return false;
}

void update_sizeop(std::vector<mlir::quantum::ValueSemanticsInstOp> &sideop,mlir::quantum::ValueSemanticsInstOp &op,int owner_user){
 
  mlir::quantum::ValueSemanticsInstOp tmp_op;
  mlir::quantum::ValueSemanticsInstOp tmp_op2;

  if(op.getNumResults() == 1){
    if(sideop.size() == 1){
      sideop.emplace_back(op);
    }else if(sideop.size() == 2){
      tmp_op = sideop.front();
      tmp_op2 = sideop.back();
      int find = 0;
      if(owner_user == 1){
        if(tmp_op.getNumResults() == 1){
          if(op.getOperand(0) == tmp_op.getResult(0)){
            *sideop.begin() = op;
            find = 1;
          }
        }else{
          if(op.getOperand(0) == tmp_op.getResult(0) || op.getOperand(0) == tmp_op.getResult(1)){
            *sideop.begin() = op;
            find = 1;
          }
        }

        if(tmp_op2.getNumResults() == 1){
          if(op.getOperand(0) == tmp_op2.getResult(0)){
            sideop.pop_back();
            sideop.emplace_back(op);
            find = 1;
          }
        }else{
          if(op.getOperand(0) == tmp_op2.getResult(0) || op.getOperand(0) == tmp_op2.getResult(1)){
            sideop.pop_back();
            sideop.emplace_back(op);
            find = 1;
          }
        }
      }else{
        if(tmp_op.getNumResults() == 1){
          if(op.getResult(0) == tmp_op.getOperand(0)){
            *sideop.begin() = op;
            find = 1;
          }
        }else{
          if(op.getResult(0) == tmp_op.getOperand(0) || op.getResult(0) == tmp_op.getOperand(1)){
            *sideop.begin() = op;
            find = 1;
          }
        }

        if(tmp_op2.getNumResults() == 1){
          if(op.getResult(0) == tmp_op2.getOperand(0)){
            sideop.pop_back();
            sideop.emplace_back(op);
            find = 1;
          }
        }else{
          if(op.getResult(0) == tmp_op2.getOperand(0) || op.getResult(0) == tmp_op2.getOperand(1)){
            sideop.pop_back();
            sideop.emplace_back(op);
            find = 1;
          }
        }
      }
      if(find == 0){
        if(tmp_op.getNumResults() == 2){
          *sideop.begin() = op;
        }
        if(tmp_op2.getNumResults() == 2){
          sideop.pop_back();
          sideop.emplace_back(op);
        }
      }
    }
  }else if(op.getNumResults() == 2){
    sideop.clear();
    sideop.emplace_back(op);
  }

}

void find_blocks(std::vector<mlir::quantum::ValueSemanticsInstOp> &current_block, 
mlir::quantum::ValueSemanticsInstOp &op,std::vector<std::pair<std::string, int64_t>> &total_index,std::vector<mlir::quantum::ValueSemanticsInstOp> &sideop,
                      int owner_or_user){
  std::vector<mlir::quantum::ValueSemanticsInstOp> list_owner;
  mlir::quantum::ValueSemanticsInstOp tmp_op1 = NULL;
  mlir::quantum::ValueSemanticsInstOp tmp_op2 = NULL;
  if(owner_or_user == 0){
    int num = op.getNumResults();
    std::vector<std::vector<mlir::quantum::ValueSemanticsInstOp>> all_owner;
    for(int i = 0;i < num;i++){
      auto operand = op.getOperand(i);
      auto operation = operand.dyn_cast_or_null<mlir::OpResult>().getOwner();
      auto owner = dyn_cast_or_null<mlir::quantum::ValueSemanticsInstOp>(operation);
      if(owner){
        if(owner.getNumResults() == 2){
          list_owner.clear();
          list_owner.emplace_back(owner);
          auto it = std::find(all_owner.begin(), all_owner.end(), list_owner);
          if(it == all_owner.end()){
            all_owner.emplace_back(list_owner);
          }
        }else{
          list_owner.clear();
          list_owner.emplace_back(owner);
          find_runs(list_owner,owner,0);
          all_owner.emplace_back(list_owner);
        }
      }
    }
    if(all_owner.size() == 0){
      return;
    }
    if(num == 2){
      auto number = op.getOperation()->getAttrOfType<mlir::IntegerAttr>(llvm::StringRef("location")).getInt();
      auto current_qbits_index = hashTableindex[number];

      bool Positive = qllvm::matrix::ForwardOrReverse(current_qbits_index,qbit_vect);
      if(!Positive){
        reverse(all_owner.begin(),all_owner.end());
      }
      if(all_owner.front().front().getNumResults() == 2 && all_owner.back().front().getNumResults() == 1){
        reverse(all_owner.begin(),all_owner.end());
      }
    }

    int i = 0;
    for(auto &owner_list: all_owner){
      auto owner = owner_list.front();
      if(judge_same_index(total_index,sideop,owner,0)){
        if(i == 0){
          tmp_op1 = owner;
          i++;
        }else{
          tmp_op2 = owner;
        }
        current_block.insert(current_block.begin(),owner_list.begin(),owner_list.end());
        for(auto &op1:owner_list){
          mlir::OpBuilder rewriter(op1);
          op1.getOperation()->setAttr(llvm::StringRef("selected"),mlir::IntegerAttr::get(rewriter.getI32Type(), 1));
        }

        update_sizeop(sideop,owner,0);
      }
    }
    
    if(tmp_op1 != NULL && tmp_op2 == NULL){
      find_blocks(current_block,tmp_op1,total_index,sideop,0);
    }else if(tmp_op1 != NULL && tmp_op2 != NULL && sideop.size() == 1){
      find_blocks(current_block,tmp_op2,total_index,sideop,0);
    }else if(tmp_op1 != NULL && tmp_op2 != NULL && sideop.size() == 2){
      auto operand = tmp_op1.getOperand(0);
      auto operation = operand.dyn_cast_or_null<mlir::OpResult>().getOwner();
      auto owner = dyn_cast_or_null<mlir::quantum::ValueSemanticsInstOp>(operation);
      auto operand2 = tmp_op2.getOperand(0);
      auto operation2 = operand2.dyn_cast_or_null<mlir::OpResult>().getOwner();
      auto owner2 = dyn_cast_or_null<mlir::quantum::ValueSemanticsInstOp>(operation2);
      if(owner != NULL && owner2 != NULL){
        if(judge_same_index(total_index,sideop,owner,0) && judge_same_index(total_index,sideop,owner2,0)){
          find_blocks(current_block,tmp_op1,total_index,sideop,0);
        }
      }
    }
    
  }else if(owner_or_user == 1){
    std::vector<std::vector<mlir::quantum::ValueSemanticsInstOp>> all_user;
    tmp_op1 = NULL;
    tmp_op2 = NULL;
    int num = op.getNumResults();
    for(int i = 0;i < num;i++){
      auto return_value = *op.result().begin();
      if(i == 1){
        return_value = op.getResults().back();
      }
      if(return_value.hasOneUse()){
        auto user = *return_value.user_begin();
        if (auto next_inst = dyn_cast_or_null<mlir::quantum::ValueSemanticsInstOp>(user)){
          if(next_inst.getNumResults() == 2){
            list_owner.clear();
            list_owner.emplace_back(next_inst);
            auto it = std::find(all_user.begin(), all_user.end(), list_owner);
            if(it == all_user.end()){
              all_user.emplace_back(list_owner);
            }
          }else{
            list_owner.clear();
            list_owner.emplace_back(next_inst);
            find_runs(list_owner,next_inst,1);
            all_user.emplace_back(list_owner);
          }
        }
      }
    }
    if(all_user.size() == 0){
      return;
    }

    if(num == 2){
      auto number = op.getOperation()->getAttrOfType<mlir::IntegerAttr>(llvm::StringRef("location")).getInt();
      auto current_qbits_index = hashTableindex[number];

      bool Positive = qllvm::matrix::ForwardOrReverse(current_qbits_index,qbit_vect);
      if(!Positive){
        reverse(all_user.begin(),all_user.end());
      }
      if(all_user.front().front().getNumResults() == 2 && all_user.back().front().getNumResults() == 1){
        reverse(all_user.begin(),all_user.end());
      }
    }

    int i = 0;
    for(auto &user_list: all_user){
      auto user = user_list.front();

      if(judge_same_index(total_index,sideop,user,1)){
      
        if(i == 0){
          tmp_op1 = user_list.back();
          i++;
        }else{
          tmp_op2 = user_list.back();
        }
        current_block.insert(current_block.end(),user_list.begin(),user_list.end());
        for(auto &op1:user_list){
          mlir::OpBuilder rewriter(op1);
          op1.getOperation()->setAttr(llvm::StringRef("selected"),mlir::IntegerAttr::get(rewriter.getI32Type(), 1));
        }
        user = user_list.back();
        update_sizeop(sideop,user,1);
      }
    }

    if(tmp_op1 != NULL && tmp_op2 == NULL){ 
      find_blocks(current_block,tmp_op1,total_index,sideop,1);
    }else if(tmp_op1 != NULL && tmp_op2 != NULL && sideop.size() == 1){
      find_blocks(current_block,tmp_op2,total_index,sideop,1);
    }else if(tmp_op1 != NULL && tmp_op2 != NULL && sideop.size() == 2){
      auto return_value = *tmp_op1.result().begin();
      auto return_value2 = *tmp_op2.result().begin();
      mlir::quantum::ValueSemanticsInstOp next_inst = NULL;
      mlir::quantum::ValueSemanticsInstOp next_inst2 = NULL;
      if(return_value.hasOneUse()) {
        auto user1 = *return_value.user_begin();
        next_inst = dyn_cast_or_null<mlir::quantum::ValueSemanticsInstOp>(user1);
      }
      if(return_value2.hasOneUse()){
        auto user2 = *return_value2.user_begin();
        next_inst2 = dyn_cast_or_null<mlir::quantum::ValueSemanticsInstOp>(user2);
      }

      if(next_inst != NULL && next_inst2 != NULL){
        if(judge_same_index(total_index,sideop,next_inst,1) && judge_same_index(total_index,sideop,next_inst2,1)){
          find_blocks(current_block,tmp_op1,total_index,sideop,1);
        }
      }
    }
  }
}

// If the matrix is finite: no NaN elements
template <typename Derived>
inline bool isFinite(const Eigen::MatrixBase<Derived> &x) {
  return ((x - x).array() == (x - x).array()).all();
}

const double TOLERANCE = 1e-9;
template <typename Derived>
bool allClose(const Eigen::MatrixBase<Derived> &in_mat1,
              const Eigen::MatrixBase<Derived> &in_mat2,
              double in_tol = TOLERANCE) {
  if (!isFinite(in_mat1) || !isFinite(in_mat2)) {
    return false;
  }

  if (in_mat1.rows() == in_mat2.rows() && in_mat1.cols() == in_mat2.cols()) {
    for (int i = 0; i < in_mat1.rows(); ++i) {
      for (int j = 0; j < in_mat1.cols(); ++j) {
        if (std::abs(in_mat1(i, j) - in_mat2(i, j)) > in_tol) {
          return false;
        }
      }
    }

    return true;
  }
  return false;
}

void del_1q_sequence(std::vector<mlir::quantum::ValueSemanticsInstOp> &run,std::vector<mlir::quantum::ValueSemanticsInstOp> &deadOps){
   /*
  run: the collection of single-qubit quantum gate blocks
  deadOps: the collection of gates to be deleted
  */
  Eigen::Matrix2cd test_matrix;
  test_matrix = totalMat(run);
  // calculate the matrix form of the single-qubit quantum gate block
  std::vector<std::string> basis_names = {};

  if(basic_gateset.size() > 0){
    basis_names.clear();
    if(basic_gateset.find("rz") != basic_gateset.end() && basic_gateset.find("ry") != basic_gateset.end()){
        basis_names.emplace_back("ZYZ");
    }
    if(basic_gateset.find("rx") != basic_gateset.end() && basic_gateset.find("ry") != basic_gateset.end()){
        basis_names.emplace_back("XYX");
    }
    if(basic_gateset.find("rz") != basic_gateset.end() && basic_gateset.find("rx") != basic_gateset.end()){
        basis_names.emplace_back("ZXZ");
        basis_names.emplace_back("XZX");
    }
    if(basic_gateset.find("sx") != basic_gateset.end() && basic_gateset.find("x") != basic_gateset.end()){
        basis_names.emplace_back("ZSXX");
    }
    if(basic_gateset.find("rz") != basic_gateset.end() && basic_gateset.find("sx") != basic_gateset.end()){
        basis_names.emplace_back("ZSX");
    }
  }

  std::vector<qllvm::utils::EulerBasis> target_basis_set;
  for(int i = 0;i < basis_names.size();i++){
      auto basis = qllvm::utils::euler_Basis_FromStr(basis_names[i]);
      target_basis_set.emplace_back(basis);
  }
  auto simplified_seq = qllvm::utils::leastcost_basis(test_matrix,target_basis_set);
  auto last = run.back();
  auto first = run.front();
    // create the quantum gate according to the generated sequence
  if(simplified_seq.size() < run.size()){
    if(simplified_seq.size() == 0){
      mlir::Value inputQubit = first->getOperand(0);
      last->getResult(0).replaceAllUsesWith(inputQubit);
      for (auto &op_to_delete : run) {
        deadOps.emplace_back(op_to_delete);
      }
      return;
    }

    auto op = last;
    std::vector<mlir::quantum::ValueSemanticsInstOp> new_ops;
    mlir::OpBuilder rewriter(op);
    rewriter.setInsertionPointAfter(last);

    for(auto &[pauli_inst, thetas]: simplified_seq){
      std::vector<mlir::Value> param_val;
      for(auto theta : thetas){
        mlir::Value theta_val = rewriter.create<mlir::ConstantOp>(
          op.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), theta));
        param_val.emplace_back(theta_val);
      }

      std::vector<mlir::Type> ret_types{first.getOperand(0).getType()};
      if(param_val.size()!=0){
        auto new_inst = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
              op.getLoc(), llvm::makeArrayRef(ret_types), pauli_inst,
              llvm::makeArrayRef(new_ops.empty() ? first.getOperand(0) : new_ops.back().getResult(0)),
              llvm::makeArrayRef(param_val));
        new_ops.emplace_back(new_inst);
      }else{
        auto new_inst = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
            op.getLoc(), llvm::makeArrayRef(ret_types), pauli_inst,
            llvm::makeArrayRef(new_ops.empty() ? first.getOperand(0) : new_ops.back().getResult(0)),
            llvm::None);
        new_ops.emplace_back(new_inst);
      }
    }
    auto last_inst_orig = run.back();
    if (new_ops.empty()) {
      auto first_inst_orig = run.front();
      (*last_inst_orig.result_begin())
          .replaceAllUsesWith(first_inst_orig.getOperand(0));
    } else {
      auto last_inst_new = new_ops.back();
      (*last_inst_orig.result_begin())
          .replaceAllUsesWith(*last_inst_new.result_begin());
    }
    for (auto &op_to_delete : run) {
      deadOps.emplace_back(op_to_delete);
    }
  }
}

std::vector<mlir::quantum::ValueSemanticsInstOp> find_first_last(std::vector<mlir::quantum::ValueSemanticsInstOp> &blocks,
std::vector<std::pair<std::string, int64_t>> &index,int i){
  std::pair<std::string, int64_t> index_1q;
  std::vector<std::pair<std::string, int64_t>> current_index;
  std::vector<mlir::quantum::ValueSemanticsInstOp> op_to_use;
  std::vector<mlir::quantum::ValueSemanticsInstOp> block;
  block = blocks;
  bool Positive;
  if(i == 1){
    reverse(block.begin(),block.end());
  }

  index.clear();
  mlir::quantum::ValueSemanticsInstOp op = block.front();
  op_to_use.emplace_back(op);

  if(op.getNumResults() == 2){
    auto number = op.getOperation()->getAttrOfType<mlir::IntegerAttr>(llvm::StringRef("location")).getInt();
    current_index = hashTableindex[number];
    Positive = qllvm::matrix::ForwardOrReverse(current_index,qbit_vect);
    if(Positive){
      index = current_index;
    }else{
      index.emplace_back(current_index.back());
      index.emplace_back(current_index.front());
    }
    return op_to_use;
  }else{
    index_1q = OP::getbit_from_valueSemanticsInstOp(op);
    index.emplace_back(index_1q);
  }

  for(auto &elem : block){
    // one single-qubit quantum gate, one two-qubit quantum gate, first store the single-qubit quantum gate, then store the two-qubit quantum gate
    if(elem.getNumResults() == 2){
      auto number = elem.getOperation()->getAttrOfType<mlir::IntegerAttr>(llvm::StringRef("location")).getInt();
      current_index = hashTableindex[number];
      index.clear();
      Positive = qllvm::matrix::ForwardOrReverse(current_index,qbit_vect);
      if(Positive){
        index = current_index;
      }else{
        index.emplace_back(current_index.back());
        index.emplace_back(current_index.front());
      }
      op_to_use.emplace_back(elem);
      break;
    }else{
      // two single-qubit gates, store in first in order
      index_1q = OP::getbit_from_valueSemanticsInstOp(elem);
      if(index_1q != index.front()){
        index.emplace_back(index_1q);
        Positive = qllvm::matrix::ForwardOrReverse(index,qbit_vect);
        if(Positive){
          op_to_use.emplace_back(elem);
        }else{
          index.pop_back();
          index.insert(index.begin(),index_1q);
          op_to_use.insert(op_to_use.begin(),elem);
        }
        break;
      }
    }
  }
  return op_to_use;
}

void find_input_param(std::vector<mlir::quantum::ValueSemanticsInstOp> &first,
std::vector<std::pair<std::string, int64_t>> &index_front,mlir::Value &inputQubit0,mlir::Value &inputQubit1){
  // find the input parameters of the block
  std::pair<std::string, int64_t> index_of_1q;
  std::vector<std::pair<std::string, int64_t>> index_of_2q;
  std::pair<std::string, int64_t> index_of_1q1;
  std::pair<std::string, int64_t> index_of_1q2;
  auto first_1op = &first.front();
  auto first_2op = &first.back();
  auto number = (first.front()).getOperation()->getAttrOfType<mlir::IntegerAttr>(llvm::StringRef("location")).getInt();
  auto number2 = (first.back()).getOperation()->getAttrOfType<mlir::IntegerAttr>(llvm::StringRef("location")).getInt();
  
  if(first.size() == 1){
    index_of_2q = hashTableindex[number];
    if(index_of_2q == index_front){
      inputQubit0 = first_1op->getOperand(0);
      inputQubit1 = first_1op->getOperand(1);
    }else{
      inputQubit0 = first_1op->getOperand(1);
      inputQubit1 = first_1op->getOperand(0);
    }
  }else{
    if(first.front().getNumResults() == 1 && first.back().getNumResults() == 1){
      index_of_1q1 = OP::getbit_from_valueSemanticsInstOp(first.front());
      index_of_1q2 = OP::getbit_from_valueSemanticsInstOp(first.back());
      inputQubit0 = first_1op->getOperand(0);
      inputQubit1 = first_2op->getOperand(0);
    }else{
      auto first_op = first.front();
      index_of_1q1 = (hashTableindex[number]).front();
      index_of_2q = hashTableindex[number2];
      if(index_of_2q == index_front){
        if(index_of_1q1 == index_front.front()){
          inputQubit0 = first_1op->getOperand(0);
          inputQubit1 = first_2op->getOperand(1);
        }else{
          inputQubit0 = first_2op->getOperand(0);
          inputQubit1 = first_1op->getOperand(0);
        }  
      }else{
        if(index_of_1q == index_front.front()){
          inputQubit0 = first_1op->getOperand(0);
          inputQubit1 = first_2op->getOperand(0);
        }else{
          inputQubit0 = first_2op->getOperand(1);
          inputQubit1 = first_1op->getOperand(0);
        }
      }
      
    }
  }
}


// sort the two gates according to the output qubits
void find_op(std::vector<mlir::quantum::ValueSemanticsInstOp> &lasts,std::vector<std::pair<std::string, int64_t>> &index_back,
mlir::quantum::ValueSemanticsInstOp &last0,mlir::quantum::ValueSemanticsInstOp &last1){
  std::vector<mlir::quantum::ValueSemanticsInstOp> last;
  last = lasts;
  
  auto last_1op = last.front();
  auto last_2op = last.back();

  auto number1 = last_1op.getOperation()->getAttrOfType<mlir::IntegerAttr>(llvm::StringRef("location")).getInt();
  auto number2 = last_2op.getOperation()->getAttrOfType<mlir::IntegerAttr>(llvm::StringRef("location")).getInt();

  if(last.size() == 1){
    last0 = last_1op;
    last1 = last_1op;
    return;
  }else if(last.front().getNumResults() == 1 && last.back().getNumResults() == 1){
    last0 = last_1op;
    last1 = last_2op;
    return;
  }else{
    auto index_of_1q = (hashTableindex[number1]).front();
    auto index_of_2q = hashTableindex[number2];
    auto Positive = qllvm::matrix::ForwardOrReverse(index_of_2q,qbit_vect);
    if(!Positive){
      if(index_of_1q == index_back.back()){
        last0 = last_2op;
        last1 = last_1op;
      }else{
        last0 = last_1op;
        last1 = last_2op;
      }
    }else{
      if(index_of_1q == index_back.front()){
        last0 = last_1op;
        last1 = last_2op;
      }else{
        last0 = last_2op;
        last1 = last_1op;
      }      
    }
  }
}

// create a new quantum gate
void creat_new_gate(mlir::quantum::ValueSemanticsInstOp &op, mlir::Value inputQubit0,mlir::Value inputQubit1,
mlir::Type qubit_type,std::vector<qllvm::kak::TMP_OP> &tmp,mlir::Value &outputQubit0,
mlir::Value &outputQubit1,std::vector<std::pair<std::string, int64_t>> index_front){
  mlir::quantum::ValueSemanticsInstOp op_new;
  mlir::quantum::ValueSemanticsInstOp new_gate;
  std::vector<mlir::quantum::ValueSemanticsInstOp> lasts;
  op_new = op;
  mlir::OpBuilder rewriter(op_new);
  rewriter.setInsertionPointAfter(op_new);
  std::string op_name;
  mlir::Value inputQubit_0index;
  mlir::Value inputQubit_1index;
  std::vector<double> param;
  std::vector<mlir::Value> param_value;
  std::vector<mlir::Value> inputQubit;
  std::vector<mlir::Type> q_type;
  int count = 0;

  for(auto &one : tmp){
    op_name = one.name;
    if(count == 0){
      inputQubit_0index = inputQubit0;
      inputQubit_1index = inputQubit1;
      count = 1;
    }

    q_type.clear();
    inputQubit.clear();
    if(one.indexs.size() == 1){
      q_type.emplace_back(qubit_type);
      if(one.indexs.front() == 0){
        inputQubit.emplace_back(inputQubit_0index);
      }else{
        inputQubit.emplace_back(inputQubit_1index);
      }
    }else{
      q_type.emplace_back(qubit_type);
      q_type.emplace_back(qubit_type);
      if(one.indexs.front() < one.indexs.back()){
        inputQubit.emplace_back(inputQubit_0index);
        inputQubit.emplace_back(inputQubit_1index);
      }else{
        inputQubit.emplace_back(inputQubit_1index);
        inputQubit.emplace_back(inputQubit_0index);
      }
    }
    param_value.clear();
    if(one.params.size() > 0){
      param = one.params;
      for(int i = 0; i < param.size();i++){
        mlir::Value index_v = rewriter.create<mlir::ConstantOp>(
                op_new.getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), param[i]));
        param_value.emplace_back(index_v);
      }
    }

    if(param_value.size() > 0){
      new_gate =
          rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
              op_new.getLoc(), llvm::makeArrayRef(q_type),
              op_name, llvm::makeArrayRef(inputQubit),
              llvm::makeArrayRef(param_value));
      lasts.emplace_back(new_gate);
    }else{
      new_gate =
          rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
              op_new.getLoc(), llvm::makeArrayRef(q_type),
              op_name, llvm::makeArrayRef(inputQubit),
              llvm::None);
      lasts.emplace_back(new_gate);
    }

    if(one.indexs.size() == 1){
      if(one.indexs.front() == 0){
        inputQubit_0index = new_gate.getResult(0);
      }else{
        inputQubit_1index = new_gate.getResult(0);
      }
    }else{
      if(one.indexs.front() < one.indexs.back()){
        inputQubit_0index = new_gate.getResult(0);
        inputQubit_1index = new_gate.getResult(1);
      }else{
        inputQubit_0index = new_gate.getResult(1);
        inputQubit_1index = new_gate.getResult(0);
      }
    }

    op_new = new_gate;
  }

  auto last = find_first_last(lasts,index_front,1);
  if(last.size() == 1){
    if(tmp.back().indexs.front() < tmp.back().indexs.back()){
      outputQubit0 = last.front().getResult(0);
      outputQubit1 = last.front().getResult(1);
    }else{
      outputQubit0 = last.front().getResult(1);
      outputQubit1 = last.front().getResult(0);
    }
  }else{
    if(last.front().getNumResults() == 1 && last.back().getNumResults() == 1){
      outputQubit0 = last.front().getResult(0);
      outputQubit1 = last.back().getResult(0);
    }else{
      auto index_new = OP::getbit_from_muti_valueSemanticsInstOp(last.back());
      auto reacreate_index = OP::getbit_from_valueSemanticsInstOp(last.front());
      if(reacreate_index == index_front.front() && index_new.back() == index_front.back()){
        outputQubit0 = last.front().getResult(0);
        outputQubit1 = last.back().getResult(1);
      }else if(reacreate_index == index_front.front() && index_new.front() == index_front.back()){
        outputQubit0 = last.front().getResult(0);
        outputQubit1 = last.back().getResult(0);
      }else if(reacreate_index == index_front.back() && index_new.front() == index_front.front()){
        outputQubit0 = last.back().getResult(0);
        outputQubit1 = last.front().getResult(0);
      }else if(reacreate_index == index_front.back() && index_new.back() == index_front.front()){
        outputQubit0 = last.back().getResult(1);
        outputQubit1 = last.front().getResult(0);
      }
    }
  }
  return;
}

// kak decompose the u3 gate
void kak_decompose_u3(std::vector<mlir::quantum::ValueSemanticsInstOp> &block,std::vector<mlir::quantum::ValueSemanticsInstOp> &deadOps){
  if(block.size() <= 1){
    return;
  }

  Eigen::MatrixXcd current_matrix;
  current_matrix = totalMat(block);

  std::vector<mlir::quantum::ValueSemanticsInstOp> first;
  std::vector<mlir::quantum::ValueSemanticsInstOp> last;
  std::vector<std::pair<std::string, int64_t>> index_front;
  std::vector<std::pair<std::string, int64_t>> index_back;
  mlir::Value inputQubit0;
  mlir::Value inputQubit1;
  first = find_first_last(block,index_front,0);
  last = find_first_last(block,index_back,1);

  std::vector<size_t> index;
  index.emplace_back(0);
  index.emplace_back(1);
  auto k = qllvm::kak::KAK();
  std::vector<qllvm::kak::TMP_OP> tmp = k.expand(current_matrix, index,basic_gateset);

  if(tmp.size() == 0){
    return;
  }

  if((tmp.size() < block.size()) || (block.size() > 20)){
    auto first0 = first.front();
    auto first1 = first.back();
    find_op(first,index_front,first0,first1);
    auto last0 = last.front();
    auto last1 = last.front();
    find_op(last,index_back,last0,last1);
    mlir::Type qubit_type = first0.getOperand(0).getType();
    mlir::Value outputQubit0;
    mlir::Value outputQubit1;


    find_input_param(first,index_front,inputQubit0,inputQubit1);
    auto loc1 = last0.getOperation()->getAttrOfType<mlir::IntegerAttr>(llvm::StringRef("location")).getInt();
    auto loc2 = last1.getOperation()->getAttrOfType<mlir::IntegerAttr>(llvm::StringRef("location")).getInt();
    if(loc1 < loc2){
      creat_new_gate(last0,inputQubit0,inputQubit1,qubit_type,tmp,outputQubit0,outputQubit1,index_front);
    }else{
      creat_new_gate(last1,inputQubit0,inputQubit1,qubit_type,tmp,outputQubit0,outputQubit1,index_front);
    }
    
    if(last.size() == 1){
      auto number1 = (last.front()).getOperation()->getAttrOfType<mlir::IntegerAttr>(llvm::StringRef("location")).getInt();
      auto index_last = hashTableindex[number1];
      if(index_last == index_back){
        last0.getResult(0).replaceAllUsesWith(outputQubit0); 
        last0.getResult(1).replaceAllUsesWith(outputQubit1); 
      }else{
        last0.getResult(0).replaceAllUsesWith(outputQubit1); 
        last0.getResult(1).replaceAllUsesWith(outputQubit0); 
      }
    }else if(last.size() == 2){
      if(last0.getNumResults() == 1 && last1.getNumResults() == 1){
        last0.getResult(0).replaceAllUsesWith(outputQubit0); 
        last1.getResult(0).replaceAllUsesWith(outputQubit1); 
      }else{
        auto number2 = (last.back()).getOperation()->getAttrOfType<mlir::IntegerAttr>(llvm::StringRef("location")).getInt();
        auto index_last = hashTableindex[number2];

        if(index_last == index_back){
          if(last0.getNumResults() == 2){
            last0.getResult(0).replaceAllUsesWith(outputQubit0); 
            last1.getResult(0).replaceAllUsesWith(outputQubit1); 
          }else{
            last0.getResult(0).replaceAllUsesWith(outputQubit0); 
            last1.getResult(1).replaceAllUsesWith(outputQubit1);
          }
        }else {
          if(last0.getNumResults() == 2){
            last0.getResult(1).replaceAllUsesWith(outputQubit0); 
            last1.getResult(0).replaceAllUsesWith(outputQubit1); 
          }else{
            last0.getResult(0).replaceAllUsesWith(outputQubit0); 
            last1.getResult(0).replaceAllUsesWith(outputQubit1);
          }
        }
      }
    }
  
    for (auto &op_to_delete : block) {
      deadOps.emplace_back(op_to_delete);
    }
    for(auto &elem: block){
      mlir::OpBuilder rewriter(elem);
      elem.getOperation()->setAttr(llvm::StringRef("selected"),mlir::IntegerAttr::get(rewriter.getI32Type(), 1));
    }
  }
}

void ConsolidateBlocks::runOnOperation() {
  int counts =0;

  basic_gateset = basic_gate;

  getOperation().walk([&](mlir::quantum::QallocOp op) {
    qbit_vect.emplace(op.name().str(),counts++);
  });
  
  std::vector<std::pair<std::string, int64_t>> idex_two;
  std::pair<std::string, int64_t> idex_one;
  int loc = 0;
  getOperation().walk([&](mlir::quantum::ValueSemanticsInstOp op) {
    idex_two.clear();
    mlir::OpBuilder rewriter(op);
    op.getOperation()->setAttr(llvm::StringRef("selected"),mlir::IntegerAttr::get(rewriter.getI32Type(), 0));
    op.getOperation()->setAttr(llvm::StringRef("onebit"),mlir::IntegerAttr::get(rewriter.getI32Type(), 0));
    op.getOperation()->setAttr(llvm::StringRef("location"),mlir::IntegerAttr::get(rewriter.getI32Type(), loc));
    
    if(op.getNumResults() == 1){
      idex_one = OP::getbit_from_valueSemanticsInstOp(op);
      idex_two.emplace_back(idex_one);
    }else if(op.getNumResults() == 2){
      idex_two = OP::getbit_from_muti_valueSemanticsInstOp(op);
    }
    hashTableindex[loc] = idex_two;
    loc++;
  });

  std::vector<std::vector<mlir::quantum::ValueSemanticsInstOp>> blocks;
  std::vector<std::vector<mlir::quantum::ValueSemanticsInstOp>> runs;
  std::vector<mlir::quantum::ValueSemanticsInstOp> deadOps;
  std::vector<mlir::quantum::ValueSemanticsInstOp> current_block;
  std::vector<mlir::quantum::ValueSemanticsInstOp> current_run;
  std::vector<std::pair<std::string, int64_t>> total_index;

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
    int onebit = op.getOperation()->getAttrOfType<mlir::IntegerAttr>(llvm::StringRef("onebit")).getInt();
    mlir::OpBuilder rewriter(op);
    if(op.getNumResults() == 1 && onebit == 0){
      op.getOperation()->setAttr(llvm::StringRef("onebit"),mlir::IntegerAttr::get(rewriter.getI32Type(), 1));
      current_run.clear();
      current_run.emplace_back(op);
      find_runs(current_run,op,0);
      find_runs(current_run,op,1);
      runs.emplace_back(current_run);
    } 
  });

  getOperation().walk([&](mlir::quantum::ValueSemanticsInstOp op) {
    mlir::OpBuilder rewriter(op);
    op.getOperation()->setAttr(llvm::StringRef("onebit"),mlir::IntegerAttr::get(rewriter.getI32Type(), 0));
  });

  getOperation().walk([&](mlir::quantum::ValueSemanticsInstOp op) {
    int selected = op.getOperation()->getAttrOfType<mlir::IntegerAttr>(llvm::StringRef("selected")).getInt();
    mlir::OpBuilder rewriter(op);
    if(op.getNumResults() == 2 && selected == 0){
      op.getOperation()->setAttr(llvm::StringRef("selected"),mlir::IntegerAttr::get(rewriter.getI32Type(), 1));
      std::vector<mlir::quantum::ValueSemanticsInstOp> sideop;
      current_block.clear();
      current_block.emplace_back(op);
      sideop.emplace_back(op);
      total_index = OP::getbit_from_muti_valueSemanticsInstOp(op);
      find_blocks(current_block,op,total_index,sideop,0);
      sideop.clear();
      sideop.emplace_back(op);
      find_blocks(current_block,op,total_index,sideop,1);
      blocks.emplace_back(current_block);
    }  
  });


  getOperation().walk([&](mlir::quantum::ValueSemanticsInstOp op) {
    mlir::OpBuilder rewriter(op);
    op.getOperation()->setAttr(llvm::StringRef("selected"),mlir::IntegerAttr::get(rewriter.getI32Type(), 0));
  });

  deadOps.clear();
  for(auto &elem:blocks){
    kak_decompose_u3(elem,deadOps);
  }
  int selected = 0;
  int stop = 0;
  for(auto &run: runs){
    stop = 0;
    selected = 0;
    if(run.size() <= 1){
      continue;
    }
    for(auto &op: run){
       selected = op.getOperation()->getAttrOfType<mlir::IntegerAttr>(llvm::StringRef("selected")).getInt();
       if(selected){
        stop = 1;
        break;
       }
    }
    if(stop == 0){
      del_1q_sequence(run,deadOps);
    }
  }
  for(auto &op : deadOps){
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
      if(before_gate_count-after_gate_count == 0 && before_circuit_depth-after_circuit_depth == 0){
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
}
// namespace qllvm