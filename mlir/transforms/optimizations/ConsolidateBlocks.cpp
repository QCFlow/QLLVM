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
#include "utils/mutation_tracking.hpp"
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
#include "llvm/Support/raw_ostream.h"
#include <iostream>
#include <string>
#include <utility>
#include <cassert>
#include "utils/circuit.hpp"
#include "utils/op.hpp"
#include "utils/gate_matrix.hpp"
#include "utils/get_matrix.hpp"
#include "utils/kak.hpp"
#include "utils/local_unitary.hpp"
#include "utils/synthesis_diagnostics.hpp"
#include "utils/synthesis_cache.hpp"
#include "CommutativeCancellationPass.hpp"
#include <algorithm>
#include <Eigen/Dense>
#include <eigen3/unsupported/Eigen/KroneckerProduct>
#include <eigen3/unsupported/Eigen/MatrixFunctions>
#include <unordered_map>
#include <tr1/unordered_map>
#include <cmath>
#include <algorithm>
#include <cstdio>
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/DenseMap.h"
#include <limits>

namespace qllvm {
using namespace std::complex_literals;
void ConsolidateBlocks::getDependentDialects(DialectRegistry &registry) const {
  registry.insert<LLVM::LLVMDialect>();
}

// Modified 2026-11-24: temporary analysis belongs to this pass invocation.
// No operation attributes or process-global tables carry collection state.
struct BlockCollectionState {
  using Wire = std::pair<std::string, int64_t>;
  std::unordered_map<std::string, int> registers;
  llvm::DenseMap<mlir::Operation *, std::vector<Wire>> wires;
  llvm::DenseMap<mlir::Operation *, size_t> order;
  llvm::DenseSet<mlir::Operation *> selected, traversedSingles;
  const std::unordered_set<std::string> &basis;
  std::string basisKey;
  explicit BlockCollectionState(const std::unordered_set<std::string> &basis)
      : basis(basis), basisKey(synthesis::basisKey(basis)) {}
};

static std::vector<std::pair<std::string, int64_t>> blockWires(
    BlockCollectionState &state, mlir::quantum::ValueSemanticsInstOp op) {
  auto found = state.wires.find(op.getOperation());
  if (found != state.wires.end()) return found->second;
  return OP::getbit_from_muti_valueSemanticsInstOp(op);
}

// calculate the matrix of single-qubit or two-qubit quantum gate block
Eigen::MatrixXcd totalMat(std::vector<mlir::quantum::ValueSemanticsInstOp> &blocks) {
  // The public single-wire helper is also used by target-specific lowering;
  // it must not depend on an earlier pass's location attributes or tables.
  if (std::all_of(blocks.begin(), blocks.end(), [](auto op) {
        return op.getNumResults() == 1;
      })) {
    Eigen::Matrix2cd accumulated = Eigen::Matrix2cd::Identity();
    for (auto it = blocks.rbegin(); it != blocks.rend(); ++it)
      accumulated = accumulated * matrix::getGateMat(*it, 0);
    return accumulated;
  }
  std::vector<std::pair<std::string, int64_t>> support;
  std::vector<std::vector<std::pair<std::string, int64_t>>> wires;
  OP::QuantumWireResolver resolver;
  for (auto op : blocks) {
    wires.emplace_back();
    for (auto value : op.qubits()) {
      auto wire = resolver.resolve(value);
      wires.back().push_back(wire);
      if (std::find(support.begin(), support.end(), wire) == support.end())
        support.push_back(wire);
    }
  }
  if (support.size() > 2) throw std::runtime_error("Local matrix support exceeds two wires");
  std::sort(support.begin(), support.end());
  Eigen::MatrixXcd accumulated = Eigen::MatrixXcd::Identity(1 << support.size(), 1 << support.size());
  for (size_t i = 0; i < blocks.size(); ++i) {
    std::vector<size_t> axes;
    for (const auto &wire : wires[i])
      axes.push_back(std::distance(support.begin(), std::find(support.begin(), support.end(), wire)));
    accumulated = matrix::embedLocalUnitary(matrix::getGateMat(blocks[i], 0), axes, support.size()) * accumulated;
  }
  return accumulated;
}

//   find the single-qubit quantum gate block
void find_runs(std::vector<mlir::quantum::ValueSemanticsInstOp> &current_run,
                mlir::quantum::ValueSemanticsInstOp op,int owner_or_user,
                llvm::DenseSet<mlir::Operation *> *visited){
  // The initial collection is read-only: use a local set instead of repeatedly
  // rebuilding operation attribute dictionaries. Other callers retain their
  // existing onebit attribute contract.
  auto firstVisit = [&](mlir::quantum::ValueSemanticsInstOp gate) {
    if (visited) return visited->insert(gate.getOperation()).second;
    auto flag = gate->getAttrOfType<mlir::IntegerAttr>("onebit");
    if (flag && flag.getInt()) return false;
    mlir::OpBuilder builder(gate);
    gate->setAttr("onebit",builder.getI32IntegerAttr(1));
    return true;
  };
  //current_run: the incomplete run sequence, op: the current gate, owner_or_user=0 find the previous gate, owner_or_user=1 find the next gate
  mlir::quantum::ValueSemanticsInstOp op_t;
  op_t = op;
  if (op.name() == "reset" || op.name() == "mz") return;
  while(true){
    auto operand = op_t.getOperand(0);
    auto operation = operand.dyn_cast_or_null<mlir::OpResult>().getOwner();
    auto owner = dyn_cast_or_null<mlir::quantum::ValueSemanticsInstOp>(operation);
    //user
    auto return_value = *op_t.result().begin();
    if(owner_or_user == 0){
      if(owner){
        if (owner->getBlock() != op_t->getBlock()) return;
        if(owner.getNumResults() == 1){
          if(owner.name() == "mz" || owner.name() == "reset"){
            return;
          }
          if(firstVisit(owner)){
            current_run.insert(current_run.begin(), owner);
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
          if (next_inst.name() == "reset" || next_inst.name() == "mz" ||
              next_inst->getBlock() != op_t->getBlock()) return;
          if(next_inst.getNumResults() == 1){
            if(firstVisit(next_inst)){
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

}

bool judge_same_index(BlockCollectionState &state,std::vector<std::pair<std::string, int64_t>> index_block,std::vector<mlir::quantum::ValueSemanticsInstOp> &sideop,mlir::quantum::ValueSemanticsInstOp &op,int owner_user){
  /*
  index_block: the index of the block
  op: the current quantum gate
  sideop: the outermost quantum gate
  owner_user:  owner_user = 0 the leftmost, owner_user = 1 the rightmost
  */

  mlir::quantum::ValueSemanticsInstOp tmp_op;
  mlir::quantum::ValueSemanticsInstOp tmp_op2;

  // to ensure that the gate does not appear in different blocks
  if(state.selected.count(op.getOperation())){
    return false;
  }
  // Modified for correctness: reset/measurement are unitary-block boundaries.
  if (op.name() == "mz" || op.name() == "reset") return false;
  // check if the index is the same
  if(op.getNumResults() == 1){
    return true;
  }

  // the current is a two-qubit quantum gate
  if(op.getNumResults() == 2){
    // get the index of the current quantum gate
    auto index = blockWires(state, op);
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

void find_blocks(BlockCollectionState &state,std::vector<mlir::quantum::ValueSemanticsInstOp> &current_block,
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
          find_runs(list_owner,owner,0,&state.traversedSingles);
          all_owner.emplace_back(list_owner);
        }
      }
    }
    if(all_owner.size() == 0){
      return;
    }
    if(num == 2){
      auto current_qbits_index = blockWires(state, op);

      bool Positive = qllvm::matrix::ForwardOrReverse(current_qbits_index,state.registers);
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
      if(judge_same_index(state,total_index,sideop,owner,0)){
        if(i == 0){
          tmp_op1 = owner;
          i++;
        }else{
          tmp_op2 = owner;
        }
        current_block.insert(current_block.begin(),owner_list.begin(),owner_list.end());
        for(auto &op1:owner_list){
          state.selected.insert(op1.getOperation());
        }

        update_sizeop(sideop,owner,0);
      }
    }

    if(tmp_op1 != NULL && tmp_op2 == NULL){
      find_blocks(state,current_block,tmp_op1,total_index,sideop,0);
    }else if(tmp_op1 != NULL && tmp_op2 != NULL && sideop.size() == 1){
      find_blocks(state,current_block,tmp_op2,total_index,sideop,0);
    }else if(tmp_op1 != NULL && tmp_op2 != NULL && sideop.size() == 2){
      auto operand = tmp_op1.getOperand(0);
      auto operation = operand.dyn_cast_or_null<mlir::OpResult>().getOwner();
      auto owner = dyn_cast_or_null<mlir::quantum::ValueSemanticsInstOp>(operation);
      auto operand2 = tmp_op2.getOperand(0);
      auto operation2 = operand2.dyn_cast_or_null<mlir::OpResult>().getOwner();
      auto owner2 = dyn_cast_or_null<mlir::quantum::ValueSemanticsInstOp>(operation2);
      if(owner != NULL && owner2 != NULL){
        if(judge_same_index(state,total_index,sideop,owner,0) && judge_same_index(state,total_index,sideop,owner2,0)){
          find_blocks(state,current_block,tmp_op1,total_index,sideop,0);
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
            find_runs(list_owner,next_inst,1,&state.traversedSingles);
            all_user.emplace_back(list_owner);
          }
        }
      }
    }
    if(all_user.size() == 0){
      return;
    }

    if(num == 2){
      auto current_qbits_index = blockWires(state, op);

      bool Positive = qllvm::matrix::ForwardOrReverse(current_qbits_index,state.registers);
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

      if(judge_same_index(state,total_index,sideop,user,1)){

        if(i == 0){
          tmp_op1 = user_list.back();
          i++;
        }else{
          tmp_op2 = user_list.back();
        }
        current_block.insert(current_block.end(),user_list.begin(),user_list.end());
        for(auto &op1:user_list){
          state.selected.insert(op1.getOperation());
        }
        user = user_list.back();
        update_sizeop(sideop,user,1);
      }
    }

    if(tmp_op1 != NULL && tmp_op2 == NULL){
      find_blocks(state,current_block,tmp_op1,total_index,sideop,1);
    }else if(tmp_op1 != NULL && tmp_op2 != NULL && sideop.size() == 1){
      find_blocks(state,current_block,tmp_op2,total_index,sideop,1);
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
        if(judge_same_index(state,total_index,sideop,next_inst,1) && judge_same_index(state,total_index,sideop,next_inst2,1)){
          find_blocks(state,current_block,tmp_op1,total_index,sideop,1);
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

// Modified 2026-11-21: cache only bit-identical one-qubit synthesis inputs
// within this pass/basis. No SSA handles or rounded angles enter the cache.
using OneQubitSynthesisCache = synthesis::Memo<std::vector<qllvm::utils::qop_t>>;
static std::string oneQubitMatrixKey(const Eigen::Matrix2cd &matrix) {
  std::string key;
  key.reserve(8 * sizeof(double));
  for (int row = 0; row < 2; ++row)
    for (int col = 0; col < 2; ++col) {
      double real = matrix(row,col).real(), imaginary = matrix(row,col).imag();
      key.append(reinterpret_cast<const char *>(&real), sizeof(real));
      key.append(reinterpret_cast<const char *>(&imaginary), sizeof(imaginary));
    }
  return key;
}
void del_1q_sequence(BlockCollectionState &state,std::vector<mlir::quantum::ValueSemanticsInstOp> &run,std::vector<mlir::quantum::ValueSemanticsInstOp> &deadOps, OneQubitSynthesisCache &cache){
   /*
  run: the collection of single-qubit quantum gate blocks
  deadOps: the collection of gates to be deleted
  */
  Eigen::Matrix2cd test_matrix;
  test_matrix = totalMat(run);
  // calculate the matrix form of the single-qubit quantum gate block
  std::vector<std::string> basis_names = {};

  if(state.basis.size() > 0){
    basis_names.clear();
    if(state.basis.find("rz") != state.basis.end() && state.basis.find("ry") != state.basis.end()){
        basis_names.emplace_back("ZYZ");
    }
    if(state.basis.find("rx") != state.basis.end() && state.basis.find("ry") != state.basis.end()){
        basis_names.emplace_back("XYX");
    }
    if(state.basis.find("rz") != state.basis.end() && state.basis.find("rx") != state.basis.end()){
        basis_names.emplace_back("ZXZ");
        basis_names.emplace_back("XZX");
    }
    if(state.basis.find("sx") != state.basis.end() && state.basis.find("x") != state.basis.end()){
        basis_names.emplace_back("ZSXX");
    }
    if(state.basis.find("rz") != state.basis.end() && state.basis.find("sx") != state.basis.end()){
        basis_names.emplace_back("ZSX");
    }
  }

  std::vector<qllvm::utils::EulerBasis> target_basis_set;
  for(int i = 0;i < basis_names.size();i++){
      auto basis = qllvm::utils::euler_Basis_FromStr(basis_names[i]);
      target_basis_set.emplace_back(basis);
  }
  const auto key = state.basisKey + oneQubitMatrixKey(test_matrix);
  std::vector<qllvm::utils::qop_t> simplified_seq;
  if (!cache.find(key, simplified_seq)) {
    simplified_seq = qllvm::utils::leastcost_basis(test_matrix,target_basis_set);
    if (test_matrix.allFinite()) cache.insert(key, simplified_seq);
  }
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

void creat_new_gate(mlir::quantum::ValueSemanticsInstOp &op, mlir::Value inputQubit0,mlir::Value inputQubit1,
mlir::Type qubit_type,std::vector<qllvm::kak::TMP_OP> &tmp,mlir::Value &outputQubit0,
mlir::Value &outputQubit1,std::vector<std::pair<std::string, int64_t>> index_front){
  if (tmp.empty()) {
    outputQubit0 = inputQubit0;
    outputQubit1 = inputQubit1;
    return;
  }
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

  // The construction loop already tracks the latest SSA value on each
  // local wire, including reversed two-qubit operands. Returning these avoids
  // tracing the newly synthesized outputs through the entire preceding circuit.
  outputQubit0 = inputQubit_0index;
  outputQubit1 = inputQubit_1index;
  return;
}

// kak decompose the u3 gate
// Modified 2026-11-19: repeated circuit blocks often have exactly the same
// local unitary. Cache only bit-identical matrices within one pass/basis; no
// rounding, approximate matching, or cached SSA handles are involved.
using SynthesisCache = synthesis::Memo<qllvm::kak::KAK::SynthesisResult>;
static std::string exactMatrixKey(const Eigen::MatrixXcd &matrix) {
  std::string key;
  key.reserve(32 * sizeof(double));
  for (int row = 0; row < 4; ++row)
    for (int col = 0; col < 4; ++col) {
      double real = matrix(row, col).real(), imag = matrix(row, col).imag();
      key.append(reinterpret_cast<const char *>(&real), sizeof(real));
      key.append(reinterpret_cast<const char *>(&imag), sizeof(imag));
    }
  return key;
}
// Modified 2026-11-22: resynthesize only a closed, straight-line SSA region.
// Every external use must be a final wire output after the insertion point.
// In particular, measurements and resets cannot become hidden interior uses.
void kak_decompose_u3(BlockCollectionState &state,std::vector<mlir::quantum::ValueSemanticsInstOp> &candidate,
                     std::vector<mlir::quantum::ValueSemanticsInstOp> &deadOps,
                     SynthesisCache &synthesisCache) {
  if (candidate.size() <= 1) return;
  auto block = candidate;
  auto *parent = block.front()->getBlock();
  llvm::DenseSet<mlir::Operation *> members;
  using Wire = std::pair<std::string, int64_t>;
  std::vector<Wire> support;
  for (auto op : block) {
    if (op->getBlock() != parent || op.name() == "mz" || op.name() == "reset" ||
        op->getParentOfType<mlir::quantum::CtrlURegion>() ||
        state.selected.count(op.getOperation()) || !members.insert(op.getOperation()).second)
      return;
    for (const auto &wire : blockWires(state, op))
      if (std::find(support.begin(), support.end(), wire) == support.end())
        support.push_back(wire);
  }
  if (support.size() != 2) return;
  std::sort(support.begin(), support.end());
  std::sort(block.begin(), block.end(), [](auto a, auto b) {
    return a->isBeforeInBlock(b.getOperation());
  });
  mlir::Value inputs[2], outputs[2];
  Eigen::MatrixXcd current = Eigen::Matrix4cd::Identity();
  int oldTwoQubit = 0;
  for (auto op : block) {
    auto wires = blockWires(state, op);
    if (wires.size() != op.getNumResults() || wires.empty() || wires.size() > 2)
      return;
    std::vector<size_t> axes;
    for (size_t j = 0; j < wires.size(); ++j) {
      size_t axis = std::distance(support.begin(),
                                std::find(support.begin(), support.end(), wires[j]));
      auto operand = op.getOperand(j);
      if (!inputs[axis]) {
        // A hidden interior producer or an interleaved wire operation makes
        // this collection unsuitable for a single unitary replacement.
        if (members.count(operand.getDefiningOp())) return;
        inputs[axis] = operand;
      } else if (outputs[axis] != operand) {
        return;
      }
      outputs[axis] = op.getResult(j);
      axes.push_back(axis);
    }
    auto local = matrix::getGateMat(op, 0);
    current = matrix::embedLocalUnitary(local, axes, 2) * current;
    oldTwoQubit += wires.size() == 2;
  }
  auto anchor = block.back();
  for (auto op : block) {
    auto wires = blockWires(state, op);
    for (size_t j = 0; j < wires.size(); ++j) {
      size_t axis = std::distance(support.begin(),
                                std::find(support.begin(), support.end(), wires[j]));
      for (auto *user : op.getResult(j).getUsers()) {
        if (members.count(user)) continue;
        if (op.getResult(j) != outputs[axis] || user->getBlock() != parent ||
            !anchor->isBeforeInBlock(user)) return;
      }
    }
  }
  const auto key = state.basisKey + exactMatrixKey(current);
  qllvm::kak::KAK::SynthesisResult result;
  if (!synthesisCache.find(key, result)) {
    result = qllvm::kak::KAK().expand(current, {0, 1}, state.basis);
    synthesisCache.insert(key, result);
  }
  if (!result) return;
  auto &gates = *result;
  int newTwoQubit = 0;
  for (const auto &gate : gates) newTwoQubit += gate.indexs.size() == 2;
  const bool accept = newTwoQubit < oldTwoQubit ||
      (newTwoQubit == oldTwoQubit && gates.size() < block.size());
  if (oldTwoQubit >= 2 && synthesis::allowDiagnostic())
    llvm::errs() << "kak block=" << block.size() << " tmp=" << gates.size()
                 << " old2=" << oldTwoQubit << " new2=" << newTwoQubit
                 << " accept=" << accept << "\n";
  if (!accept) return;
  mlir::Value replacement0, replacement1;
  creat_new_gate(anchor, inputs[0], inputs[1], inputs[0].getType(), gates,
                 replacement0, replacement1, support);
  outputs[0].replaceAllUsesWith(replacement0);
  outputs[1].replaceAllUsesWith(replacement1);
  for (auto op : block) {
    state.selected.insert(op.getOperation());
    deadOps.push_back(op);
  }
}

void ConsolidateBlocks::runOnOperation() {
  BlockCollectionState state(basic_gate);
  int counts =0;


  getOperation().walk([&](mlir::quantum::QallocOp op) {
    state.registers.emplace(op.name().str(),counts++);
  });

  std::vector<std::pair<std::string, int64_t>> idex_two;
  std::pair<std::string, int64_t> idex_one;
  int loc = 0;
  {
    OP::QuantumWireResolver wires;
    getOperation().walk([&](mlir::quantum::ValueSemanticsInstOp op) {
      idex_two.clear();
      state.order[op.getOperation()] = loc;

      if(op.getNumResults() == 1){
        idex_one = wires.resolve(op.qubits()[0]);
        idex_two.emplace_back(idex_one);
      }else if(op.getNumResults() == 2){
        for (auto value : op.qubits()) idex_two.push_back(wires.resolve(value));
      }
      state.wires[op.getOperation()] = idex_two;
      loc++;
    });
  } // No cached SSA values survive into block resynthesis.

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

  llvm::DenseSet<mlir::Operation *> visitedSingles;
  getOperation().walk([&](mlir::quantum::ValueSemanticsInstOp op) {
    if(op.getNumResults() == 1 && !visitedSingles.count(op.getOperation()) && op.name() != "reset" && op.name() != "mz" &&
       !op->getParentOfType<mlir::quantum::CtrlURegion>()){
      visitedSingles.insert(op.getOperation());
      current_run.clear();
      current_run.emplace_back(op);
      find_runs(current_run,op,0,&visitedSingles);
      find_runs(current_run,op,1,&visitedSingles);
      runs.emplace_back(current_run);
    }
  });

  visitedSingles.clear();

  getOperation().walk([&](mlir::quantum::ValueSemanticsInstOp op) {
    int selected = state.selected.count(op.getOperation());
    if(op.getNumResults() == 2 && selected == 0 &&
       !op->getParentOfType<mlir::quantum::CtrlURegion>()){
      state.selected.insert(op.getOperation());
      std::vector<mlir::quantum::ValueSemanticsInstOp> sideop;
      current_block.clear();
      current_block.emplace_back(op);
      sideop.emplace_back(op);
      // This collection phase has not changed SSA wiring since the lookup
      // table above was built; avoid tracing every wire back to its allocation.
      total_index = blockWires(state, op);
      find_blocks(state,current_block,op,total_index,sideop,0);
      sideop.clear();
      sideop.emplace_back(op);
      find_blocks(state,current_block,op,total_index,sideop,1);
      blocks.emplace_back(current_block);
    }
  });


  state.selected.clear();

  deadOps.clear();
  SynthesisCache localSynthesisCache;
  auto &synthesisCache = synthesis::currentCache ? synthesis::currentCache->matrices2q : localSynthesisCache;
  for(auto &elem:blocks){
    kak_decompose_u3(state,elem,deadOps,synthesisCache);
  }

  // The SSA block walk stops when a single-qubit gate sits on the other
  // wire of a CX. Collect each qubit pair in program order instead, skipping
  // gates that do not touch the pair and stopping at a gate that shares only
  // one of its qubits. Resynthesis still refuses to add entanglers.
  // Modified 2026-11-21: membership is queried once for every remaining gate.
  llvm::DenseSet<mlir::Operation *> deleted;
  for (auto op : deadOps) deleted.insert(op.getOperation());
  std::vector<mlir::quantum::ValueSemanticsInstOp> ordered;
  getOperation().walk([&](mlir::quantum::ValueSemanticsInstOp op) {
    // Keep nonunitary/controlled gates in the wire index as boundaries.
    // Omitting them can create a long candidate that the SSA-closure guard
    // must reject in full, losing valid segments on either side.
    if (!state.order.count(op.getOperation()) || state.selected.count(op.getOperation())) return;
    if (deleted.count(op.getOperation())) return;
    ordered.emplace_back(op);
  });
  // walk order matches the original monotonically assigned locations;
  // no attribute writes or subsequent sorting are needed.
  // Index each wire once. Walking the merge of the two support lists visits
  // exactly the same relevant gates as the old full suffix scan, in order,
  // without repeatedly traversing disjoint wires. Preserve taken-gate handling
  // and the first gate that touches the pair but also another wire.
  using Wire = std::pair<std::string, int64_t>;
  std::map<Wire, std::vector<size_t>> positions;
  for (size_t i = 0; i < ordered.size(); ++i) {
    const auto &wires = state.wires.find(ordered[i].getOperation())->second;
    for (const auto &wire : wires) positions[wire].push_back(i);
  }
  std::vector<char> taken(ordered.size(), 0);
  for (size_t i = 0; i < ordered.size(); ++i) {
    if (taken[i] || ordered[i].getNumResults() != 2 ||
        ordered[i]->getParentOfType<mlir::quantum::CtrlURegion>()) continue;
    auto pair = state.wires.find(ordered[i].getOperation())->second;
    if (pair.size() != 2) continue;
    std::vector<mlir::quantum::ValueSemanticsInstOp> pairBlock;
    const auto &firstPositions = positions.at(pair[0]);
    const auto &secondPositions = positions.at(pair[1]);
    auto first = std::lower_bound(firstPositions.begin(), firstPositions.end(), i);
    auto second = std::lower_bound(secondPositions.begin(), secondPositions.end(), i);
    while (first != firstPositions.end() || second != secondPositions.end()) {
      const size_t nextFirst = first == firstPositions.end() ? ordered.size() : *first;
      const size_t nextSecond = second == secondPositions.end() ? ordered.size() : *second;
      const size_t j = std::min(nextFirst, nextSecond);
      if (nextFirst == j) ++first;
      if (nextSecond == j) ++second;
      if (taken[j] || ordered[j]->getBlock() != ordered[i]->getBlock() ||
          ordered[j].name() == "mz" || ordered[j].name() == "reset" ||
          ordered[j]->getParentOfType<mlir::quantum::CtrlURegion>()) break;
      auto qs = state.wires.find(ordered[j].getOperation())->second;
      bool touches = false, outside = false;
      for (const auto &qubit : qs) {
        if (qubit == pair[0] || qubit == pair[1]) touches = true;
        else outside = true;
      }
      if (touches && outside) break;
      if (!touches) continue;
      pairBlock.emplace_back(ordered[j]);
      taken[j] = 1;
    }
    int entanglers = 0;
    for (auto gate : pairBlock)
      if (gate.getNumResults() == 2) ++entanglers;
    if (entanglers >= 2) {
      if (synthesis::allowDiagnostic())
        llvm::errs() << "pair n=" << pairBlock.size() << " e=" << entanglers << "\n";
      kak_decompose_u3(state,pairBlock, deadOps, synthesisCache);
    }
  }
  OneQubitSynthesisCache localOneQubitCache;
  auto &oneQubitCache = synthesis::currentCache ? synthesis::currentCache->matrices1q : localOneQubitCache;
  int selected = 0;
  int stop = 0;
  for(auto &run: runs){
    stop = 0;
    selected = 0;
    if(run.size() <= 1){
      continue;
    }
    for(auto &op: run){
       selected = state.selected.count(op.getOperation());
       if(selected){
        stop = 1;
        break;
       }
    }
    if(stop == 0){
      del_1q_sequence(state,run,deadOps,oneQubitCache);
    }
  }
  for(auto &op : deadOps){
    op->dropAllUses();
    optimization::markChanged();
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
