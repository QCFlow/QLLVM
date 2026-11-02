
/*
 * This code is part of QLLVM.
 * Modified by QCFlow (2026): avoid lookup copies and group scans, add exact
 * CX fast paths, and preserve circuit order when cancelling or merging gates.
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
#include "CommutativeCancellationPass.hpp"
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
#include "utils/get_matrix.hpp"
#include "gen_qasm.hpp"
#include "utils/op.hpp"
#include <Eigen/Dense>
#include <eigen3/unsupported/Eigen/KroneckerProduct>
#include <unordered_map>
#include <tr1/unordered_map>
#include <stdexcept>
#include <algorithm>

namespace qllvm {
void CommutativeCancellationPass::getDependentDialects(
    DialectRegistry &registry) const {
  registry.insert<LLVM::LLVMDialect>();
}

template<typename K, typename V>
class BiMap {
public:
    //  insert key-value pair
    BiMap() = default;
    void insert(const K& key, const V& value) {
        if (key_to_value.find(key) != key_to_value.end()) {
            throw std::invalid_argument("Duplicated key or value");
        }
        key_to_value[key] = value;
        value_to_key[value] = key;
    }

    //  according to key to find value
    V getValue(const K& key) const {
        auto it = key_to_value.find(key);
        if (it != key_to_value.end()) {
            return it->second;
        }
        throw std::out_of_range("Key not found");
    }

    //  according to value to find key
    K getKey(const V& value) const {
        auto it = value_to_key.find(value);
        if (it != value_to_key.end()) {
            return it->first;
        }
        throw std::out_of_range("Value not found");
    }

    //    according to a key to delete
    void removeByKey(const K& key) {
        auto it = key_to_value.find(key);
        if (it != key_to_value.end()) {
            value_to_key.erase(it->second);
            key_to_value.erase(it);
        }
    }

    //  according to a value to delete
    void removeByValue(const V& value) {
        auto it = value_to_key.find(value);
        if (it != value_to_key.end()) {
            key_to_value.erase(it->second);
            value_to_key.erase(it);
        }
    }

    //  check if the key is contained
    bool containsKey(const K& key) const {
        return key_to_value.find(key) != key_to_value.end();
    }

    //  check if the value is contained
    bool containsValue(const V& value) const {
        return value_to_key.find(value) != value_to_key.end();
    }

    std::vector<K> getKeys() const {
      std::vector<K> keys;
      for (const auto& pair : key_to_value) {
          keys.emplace_back(pair.first);
      }
      // std::reverse(keys.begin(), keys.end());
      return keys;
    }

private:
    std::map<K, V> key_to_value;
    std::map<V, K> value_to_key;
};

std::unordered_map<std::string, int> qbit_seq;
std::vector<std::pair<std::string, int64_t>> wire_set;
std::unordered_map<int, std::vector<std::pair<std::string, int64_t>>> hashTable;

// A two-qubit gate belongs to a commutation group on each of its wires.
// Build this reverse index once instead of scanning all groups per candidate.
using CommutationGroupIndex =
    std::map<std::pair<std::string, int64_t>,
             std::unordered_map<mlir::Operation *, int>>;


bool CommutationChecker(mlir::quantum::ValueSemanticsInstOp op1,mlir::quantum::ValueSemanticsInstOp op2,
  const std::unordered_map<int, std::vector<std::pair<std::string, int64_t>>> &hash_Table,const std::unordered_map<std::string, int> &qbitseq){
  if(op1.name() == "mz" || op2.name() == "mz"){
    return false;
  }
  auto qbits_op1 = op1.getNumResults();
  auto qbits_op2 = op2.getNumResults();
  std::vector<std::pair<std::string, int64_t>> index_two_1;
  std::vector<std::pair<std::string, int64_t>> index_two_2;
  std::pair<std::string, int64_t> index_one_1;
  std::pair<std::string, int64_t> index_one_2;

  Eigen::MatrixXcd mat_op1;
  Eigen::MatrixXcd mat_op2;


  auto number1 = op1.getOperation()->getAttrOfType<mlir::IntegerAttr>(llvm::StringRef("number")).getInt();
  auto number2 = op2.getOperation()->getAttrOfType<mlir::IntegerAttr>(llvm::StringRef("number")).getInt();
  const auto &temp1 = hash_Table.at(number1);
  const auto &temp2 = hash_Table.at(number2);

  // Check common gates before allocating matrices. These identities are exact;
  // in particular RZ commutes with the control, RX with the target of CX.
  const auto name1 = op1.name();
  const auto name2 = op2.name();
  const bool cx1 = name1 == "cx" || name1 == "cnot";
  const bool cx2 = name2 == "cx" || name2 == "cnot";
  if (cx1 && cx2 && temp1.size() == 2 && temp2.size() == 2)
    return temp1.front() != temp2.back() && temp2.front() != temp1.back();
  if (cx1 && temp1.size() == 2 && temp2.size() == 1) {
    if (name2 == "rz" && temp2.front() == temp1.front()) return true;
    if (name2 == "rx" && temp2.front() == temp1.back()) return true;
  }
  if (cx2 && temp2.size() == 2 && temp1.size() == 1) {
    if (name1 == "rz" && temp1.front() == temp2.front()) return true;
    if (name1 == "rx" && temp1.front() == temp2.back()) return true;
  }

  const auto isControlledPauli = [](llvm::StringRef name) {
    return name == "cx" || name == "cnot" || name == "cy" || name == "cz";
  };

  if(qbits_op1 == 1){
    // index_one_1 = OP::getbit_from_valueSemanticsInstOp(op1);
    index_one_1 = temp1.front();
    mat_op1 = qllvm::matrix::getGateMat(op1,0);
  }else{
    // index_two_1 = OP::getbit_from_muti_valueSemanticsInstOp(op1);
    index_two_1 = temp1;
    if(qllvm::matrix::ForwardOrReverse(index_two_1,qbitseq)){
      mat_op1 = qllvm::matrix::getGateMat(op1,0);
    }else{
      mat_op1 = qllvm::matrix::getGateMat(op1,1);
    }
  }

  if(qbits_op2 == 1){
    // index_one_2 = OP::getbit_from_valueSemanticsInstOp(op2);
    index_one_2 = temp2.front();
    mat_op2 = qllvm::matrix::getGateMat(op2,0);
  }else{
    // index_two_2 = OP::getbit_from_muti_valueSemanticsInstOp(op2);
    index_two_2 = temp2;
    if(qllvm::matrix::ForwardOrReverse(index_two_2,qbitseq)){
      mat_op2 = qllvm::matrix::getGateMat(op2,0);
    }else{
      mat_op2 = qllvm::matrix::getGateMat(op2,1);
    }
  }

  Eigen::MatrixXcd l_result;
  Eigen::MatrixXcd r_result;
  Eigen::MatrixXcd operator_1;
  Eigen::MatrixXcd operator_2;
  std::vector<std::pair<std::string, int64_t>> tmp_two;

  int64_t extra_qarg;
  int64_t size;
  Eigen::MatrixXcd id_op = Eigen::MatrixXcd::Identity(2, 2);

  if(qbits_op1 == qbits_op2){
    if(qbits_op1 == 1){
      l_result = mat_op1 * mat_op2;//op1 * op2
      r_result = mat_op2 * mat_op1;//op2 * op1
    }else{
      if(index_two_2 == index_two_1 || (index_two_2.front() == index_two_1.back() && index_two_2.back() == index_two_1.front())){
        l_result = mat_op1 * mat_op2;//op1 * op2
        r_result = mat_op2 * mat_op1;//op2 * op1
      }else if(index_two_2.front() == index_two_1.front() && index_two_2.back() != index_two_1.back()){
        // Sharing an operand is not sufficient for arbitrary two-qubit gates.
        return isControlledPauli(name1) && isControlledPauli(name2);
      }else if(index_two_1.back() == index_two_2.back() && index_two_1.front() != index_two_2.front()){
        // CX and CZ with a shared target do NOT commute. Only apply this
        // shortcut to identical controlled Pauli axes.
        return name1 == name2 && isControlledPauli(name1);
      }else{
        if(index_two_1.back() == index_two_2.front() && index_two_1.front() != index_two_2.back()){
          tmp_two.clear();
          tmp_two.emplace_back(index_two_1.front());
          tmp_two.emplace_back(index_two_2.back());
          if(qllvm::matrix::ForwardOrReverse(index_two_1,qbit_seq) && qllvm::matrix::ForwardOrReverse(tmp_two,qbit_seq)){
            operator_1 = Eigen::kroneckerProduct(id_op,mat_op1);
            operator_2 = Eigen::kroneckerProduct(mat_op2,id_op);
            l_result = operator_1 * operator_2;
            r_result = operator_2 * operator_1;
          }else if(!qllvm::matrix::ForwardOrReverse(index_two_1,qbit_seq) && !qllvm::matrix::ForwardOrReverse(tmp_two,qbit_seq)){
            operator_1 = Eigen::kroneckerProduct(mat_op1,id_op);
            operator_2 = Eigen::kroneckerProduct(id_op,mat_op2);
            l_result = operator_1 * operator_2;
            r_result = operator_2 * operator_1;
          }else{
            return false;
          }
        }else if(index_two_1.front() == index_two_2.back() && index_two_1.back() != index_two_2.front()){
          tmp_two.clear();
          tmp_two.emplace_back(index_two_1.back());
          tmp_two.emplace_back(index_two_2.front());
          if(qllvm::matrix::ForwardOrReverse(index_two_1,qbit_seq) && !qllvm::matrix::ForwardOrReverse(tmp_two,qbit_seq)){
            operator_1 = Eigen::kroneckerProduct(mat_op1,id_op);
            operator_2 = Eigen::kroneckerProduct(id_op,mat_op2);
            l_result = operator_1 * operator_2;
            r_result = operator_2 * operator_1;
          }else if(!qllvm::matrix::ForwardOrReverse(index_two_1,qbit_seq) && qllvm::matrix::ForwardOrReverse(tmp_two,qbit_seq)){
            operator_1 = Eigen::kroneckerProduct(id_op,mat_op1);
            operator_2 = Eigen::kroneckerProduct(mat_op2,id_op);
            l_result = operator_1 * operator_2;
            r_result = operator_2 * operator_1;
          }else{
            return false;
          }
        }else{
          return false;
        }
      }
    }
      
  }else {
    if(qbits_op1 > qbits_op2){
      if(qllvm::matrix::ForwardOrReverse(index_two_1,qbitseq)){
        if(index_one_2 == index_two_1.front()){
          operator_1 = Eigen::kroneckerProduct(id_op,mat_op2);
        }else{
          operator_1 = Eigen::kroneckerProduct(mat_op2,id_op);
        }
        l_result = operator_1 * mat_op1;
        r_result = mat_op1 * operator_1;
      }else{
        if(index_one_2 == index_two_1.front()){
          operator_1 = Eigen::kroneckerProduct(mat_op2,id_op);
        }else{
          operator_1 = Eigen::kroneckerProduct(id_op,mat_op2);
        }
        l_result = operator_1 * mat_op1;
        r_result = mat_op1 * operator_1;
      }
    }else{
      if(qllvm::matrix::ForwardOrReverse(index_two_2,qbitseq)){
        if(index_one_1 == index_two_2.front()){
          operator_1 = Eigen::kroneckerProduct(id_op,mat_op1);
        }else{
          operator_1 = Eigen::kroneckerProduct(mat_op1,id_op);
        }
        l_result = operator_1 * mat_op2;
        r_result = mat_op2 * operator_1;
      }else{
        if(index_one_1 == index_two_2.front()){
          operator_1 = Eigen::kroneckerProduct(mat_op1,id_op);
        }else{
          operator_1 = Eigen::kroneckerProduct(id_op,mat_op1);
        }
        l_result = operator_1 * mat_op2;
        r_result = mat_op2 * operator_1;
      }
    }
    
  }

  if(l_result == r_result){
    return true;
  }else {
    return false;
  }
}

void findcommutativegate(mlir::quantum::ValueSemanticsInstOp op,std::vector<mlir::quantum::ValueSemanticsInstOp> &commutation_ops,
std::vector<BiMap<mlir::quantum::ValueSemanticsInstOp, std::pair<std::string, int64_t>>> &sub_commutation_vect, std::pair<std::string, int64_t> &wire, int &index){
  bool does_commute;
  std::vector<std::pair<std::string, int64_t>> index_2bit;
  std::pair<std::string, int64_t> index_1bit;
  mlir::Value return_value;

  bool cont = true;
  while(cont){
    does_commute = false;
    auto number = op.getOperation()->getAttrOfType<mlir::IntegerAttr>(llvm::StringRef("number")).getInt();
    if(op.getNumResults() == 2){
      index_2bit = hashTable[number];
      if(index_2bit.front() == wire){
        return_value = op.getResult(0);
      }else{
        return_value = op.getResult(1);
      }
    }else if(op.getNumResults() == 1){
      return_value = op.getResult(0);
    }

    if(return_value.hasOneUse()){
      auto user = *return_value.user_begin();
      if(auto next_inst = dyn_cast_or_null<mlir::quantum::ValueSemanticsInstOp>(user)) {
        for(auto &elem: commutation_ops){
          does_commute = CommutationChecker(next_inst,elem,hashTable,qbit_seq);
          if(!does_commute){
            break;
          }
        }
        
        if(does_commute){
          sub_commutation_vect[index].insert(next_inst,wire);
          commutation_ops.emplace_back(next_inst);
        }else{

          index = index + 1;
          commutation_ops.clear();
          commutation_ops.emplace_back(next_inst);
          BiMap<mlir::quantum::ValueSemanticsInstOp, std::pair<std::string, int64_t>> sub_commutation_new;
          sub_commutation_new.insert(next_inst,wire);
          sub_commutation_vect.emplace_back(sub_commutation_new);
        }
        op = next_inst;
      }else{
        cont = false;
      }
    }else{
      cont = false;
    }
  }
}

int find_controled_index(const CommutationGroupIndex &groups,
mlir::quantum::ValueSemanticsInstOp key,const std::vector<std::pair<std::string, int64_t>> &wire_site){
  return groups.at(wire_site.back()).at(key.getOperation());
}

void deal_2gates(std::vector<mlir::quantum::ValueSemanticsInstOp> &vect,const CommutationGroupIndex &groups,
  std::vector<std::vector<mlir::quantum::ValueSemanticsInstOp>> &cx,std::pair<std::string, int64_t> &wire){
  std::vector<std::vector<std::pair<std::string, int64_t>>> qbits_index;
  std::vector<std::pair<std::string, int64_t>> current_index;
  std::vector<std::pair<std::string, int64_t>> another_index;
  std::vector<mlir::quantum::ValueSemanticsInstOp> deadop1;
  std::vector<mlir::quantum::ValueSemanticsInstOp> temp_ops;
  std::vector<int> dealed(vect.size(),1);
  int new_index;
  int index;

  for(int i = 0;i < vect.size();i++){
    temp_ops.clear();
    auto op = vect[i];
    // current_index = OP::getbit_from_muti_valueSemanticsInstOp(op);
    auto number1 = op.getOperation()->getAttrOfType<mlir::IntegerAttr>(llvm::StringRef("number")).getInt();
    current_index = hashTable[number1];

    if(current_index.front() != wire || dealed[i] == 0){
      continue;
    }
    index = find_controled_index(groups,op,current_index);
    dealed[i] = 0;
    temp_ops.emplace_back(op);
    for(int j = i+1;j < vect.size();j++){
      auto op_2 = vect[j];
      // another_index = OP::getbit_from_muti_valueSemanticsInstOp(op_2);
      auto number2 = op_2.getOperation()->getAttrOfType<mlir::IntegerAttr>(llvm::StringRef("number")).getInt();
      another_index = hashTable[number2];

      if(another_index == current_index){
        new_index = find_controled_index(groups,op_2,another_index);
        if(new_index == index){
          temp_ops.emplace_back(op_2);
          dealed[j] = 0;
        }else{
          break;
        }
      }else{
        // All entries in vect already commute on the control wire. A gate
        // with another target must not hide a later identical cancellable CX.
        continue;
      }
    }

    if(temp_ops.size() > 1){
      cx.emplace_back(temp_ops);
    }
  }
}

void classification(const CommutationGroupIndex &groups,
std::vector<BiMap<mlir::quantum::ValueSemanticsInstOp, std::pair<std::string, int64_t>>> &vect,
std::vector<std::vector<mlir::quantum::ValueSemanticsInstOp>> &h,std::vector<std::vector<mlir::quantum::ValueSemanticsInstOp>> &y,
std::vector<std::vector<mlir::quantum::ValueSemanticsInstOp>> &z,std::vector<std::vector<mlir::quantum::ValueSemanticsInstOp>> &x,
std::vector<std::vector<mlir::quantum::ValueSemanticsInstOp>> &cx,std::vector<std::vector<mlir::quantum::ValueSemanticsInstOp>>&cy,
std::vector<std::vector<mlir::quantum::ValueSemanticsInstOp>> &cz,std::pair<std::string, int64_t> wire){

  std::vector<mlir::quantum::ValueSemanticsInstOp> cx_gate;//["cx", "cy", "cz", "h", "y"]
  std::vector<mlir::quantum::ValueSemanticsInstOp> cy_gate;
  std::vector<mlir::quantum::ValueSemanticsInstOp> cz_gate;
  std::vector<mlir::quantum::ValueSemanticsInstOp> h_gate;
  std::vector<mlir::quantum::ValueSemanticsInstOp> y_gate;
  std::vector<mlir::quantum::ValueSemanticsInstOp> z_rotation;//["p", "z", "u1", "rz", "t", "s"]
  std::vector<mlir::quantum::ValueSemanticsInstOp> x_rotation;//["rx", "x"]

  std::vector<std::vector<mlir::quantum::ValueSemanticsInstOp>> node_wire;
  mlir::quantum::ValueSemanticsInstOp tmp_op;
  for(auto &sub_vec: vect){
    node_wire.emplace_back(sub_vec.getKeys());
    // BiMap iterates by operation address, not circuit order. Rewiring a run
    // in that order can create a cycle or erase an operation still in use.
    auto &ordered = node_wire.back();
    std::sort(ordered.begin(), ordered.end(), [](auto lhs, auto rhs) {
      return lhs->template getAttrOfType<mlir::IntegerAttr>("number").getInt() <
             rhs->template getAttrOfType<mlir::IntegerAttr>("number").getInt();
    });
  }

  for(int index = 0;index < node_wire.size();index++){
    h_gate.clear();
    y_gate.clear();
    z_rotation.clear();
    x_rotation.clear();
    cx_gate.clear();
    cy_gate.clear();
    cz_gate.clear();
    if(node_wire[index].size() <= 1){
      continue;
    }
    for(auto &tmp_op: node_wire[index]){
      if(tmp_op.name() == "h"){
        h_gate.emplace_back(tmp_op);
      }else if(tmp_op.name() == "y"){
        y_gate.emplace_back(tmp_op);
      }else if(tmp_op.name()=="p"||tmp_op.name()=="z"||tmp_op.name()=="u1"||
              tmp_op.name()=="rz"||tmp_op.name()=="t"||tmp_op.name()=="s"){  
        z_rotation.emplace_back(tmp_op);
      }else if(tmp_op.name()=="rx"||tmp_op.name()=="x"){
        x_rotation.emplace_back(tmp_op);
      }else if(tmp_op.name()=="cx"){
        cx_gate.emplace_back(tmp_op);
      }else if(tmp_op.name()=="cy"){
        cy_gate.emplace_back(tmp_op);
      }else if(tmp_op.name()=="cz"){
        cz_gate.emplace_back(tmp_op);
      }
    }
    if(cx_gate.size() > 1){
      deal_2gates(cx_gate,groups,cx,wire);
    }
    if(cy_gate.size() > 1){
      deal_2gates(cy_gate,groups,cy,wire);
    }
    if(cz_gate.size() > 1){
      deal_2gates(cz_gate,groups,cz,wire);
    }
    if(h_gate.size() > 1){
      h.emplace_back(h_gate);
    }
    if(y_gate.size() > 1){
      y.emplace_back(y_gate);
    }
    if(z_rotation.size() > 1){
      z.emplace_back(z_rotation);
    }
    if(x_rotation.size() > 1){
      x.emplace_back(x_rotation);
    }
  }
}

double total_angle(const std::vector<mlir::quantum::ValueSemanticsInstOp> &ops){
  double total_angle = 0,current_angle = 0;
  for(auto op : ops){
    if(op.name() == "p"||op.name() == "u1"||op.name() == "rz"||op.name() == "rx"){
      current_angle = qllvm::OP::tryGetConstAngle(op.getOperand(1));
    }else if(op.name() == "z"||op.name() == "x"){
      current_angle = llvm::numbers::pi;
    }else if(op.name() == "t"){
      current_angle = llvm::numbers::pi/4;
    }else if(op.name() == "s"){
      current_angle = llvm::numbers::pi/2;
    }
    total_angle += current_angle;
  }
  double result = std::fmod(total_angle,2*M_PI);
  return result;
}

void replace_x_z(std::vector<mlir::quantum::ValueSemanticsInstOp> &vect,std::string right_name){
  auto first = &vect.front();
  auto last = &vect.back();
  double angle = total_angle(vect);
  mlir::Value inputQubit = first->getOperand(0);

  for(auto it = vect.begin(); it != vect.end(); ++it){
    auto current_op = *it;
    if(it + 1 != vect.end()){
      auto next_op = *(it+1);
      if(current_op.getResult(0) != next_op.getOperand(0)){
        current_op.getResult(0).replaceAllUsesWith(inputQubit);
        inputQubit = next_op.getOperand(0);
      }  
    } 
  }

  mlir::OpBuilder rewriter(*last);
  rewriter.setInsertionPointAfter(*last);
    
  mlir::Value theta = rewriter.create<mlir::ConstantOp>(
                    last->getLoc(), mlir::FloatAttr::get(rewriter.getF64Type(), angle));
  mlir::Type qubit_type = first->getOperand(0).getType();

  auto new_inst = rewriter.create<mlir::quantum::ValueSemanticsInstOp>(
                      last->getLoc(), llvm::makeArrayRef({qubit_type}),
                      right_name, llvm::makeArrayRef({inputQubit}),
                      llvm::makeArrayRef({theta}));
  last->getResult(0).replaceAllUsesWith(*new_inst.result_begin());
}

void delete_gate(std::vector<mlir::quantum::ValueSemanticsInstOp> &dead,std::string name1){
  if(dead.size() <= 1){
    return;
  }

  int64_t q_gate_length = dead.size();
  int64_t q_gate_length_enev = q_gate_length / 2 * 2;
  auto second = dead[dead.size() - 2];
  mlir::Value input0;
  mlir::Value input1;
  input0 = dead.front().getOperand(0);
  if(name1 == "cx" || name1 == "cy" || name1 == "cz"){
    input1 = dead.front().getOperand(1);
  }
  for(auto it = dead.begin(); it != dead.end(); ++it){
    auto current_op = *it;
    if(it + 1 != dead.end()){
      auto next_op = *(it+1);
      if((q_gate_length_enev != q_gate_length) && (current_op == second)){
        break;
      }
      if(current_op.getResult(0) != next_op.getOperand(0)){
        current_op.getResult(0).replaceAllUsesWith(input0);
        input0 = next_op.getOperand(0);
      }
      if(name1 == "cx" || name1 == "cy"||name1 == "cz"){
        if(current_op.getResult(1) != next_op.getOperand(1)){
          current_op.getResult(1).replaceAllUsesWith(input1);
          input1 = next_op.getOperand(1);
        }
      }
    } 
  }

  if(q_gate_length_enev == q_gate_length){
    dead.back().getResult(0).replaceAllUsesWith(input0);
    if(name1 == "cx" || name1 == "cy"||name1 == "cz"){
      dead.back().getResult(1).replaceAllUsesWith(input1);
    }
    for(auto &op : dead){
      op->dropAllUses();
      op.erase();
    }
  }else if(dead.size() > 2){
    second.getResult(0).replaceAllUsesWith(input0);
    if(name1 == "cx" || name1 == "cy"||name1 == "cz"){
      second.getResult(1).replaceAllUsesWith(input1);
    }
    for(auto &op1 : dead){  
      if(op1 != dead.back()){
        op1->dropAllUses();
        op1.erase();
      }
    }
  }
}

void deal_topop(const std::vector<mlir::quantum::ValueSemanticsInstOp> &topop,const std::unordered_map<int, std::vector<std::pair<std::string, int64_t>>> &hash_Table,
                std::vector<mlir::quantum::ValueSemanticsInstOp> &top_op,std::vector<std::pair<std::string, int64_t>> &wireset,
              const std::unordered_map<std::string, int> &qbitseq){
  std::vector<std::pair<std::string, int64_t>> idex_two;
  for(auto op : topop){
    int number = op.getOperation()->getAttrOfType<mlir::IntegerAttr>(llvm::StringRef("number")).getInt();
    const auto &temp1 = hash_Table.at(number);
    if(op == topop.front()){
      top_op.emplace_back(op);
      if(op.getNumResults() == 1){
        wireset.emplace_back(temp1.front());
      }else if(op.getNumResults() == 2){
        auto operand = op.getOperand(0);
        auto operation = operand.dyn_cast_or_null<mlir::OpResult>().getOwner();
        auto owner = dyn_cast_or_null<mlir::quantum::ValueSemanticsInstOp>(operation);
        operand = op.getOperand(1);
        operation = operand.dyn_cast_or_null<mlir::OpResult>().getOwner();
        auto owner2 = dyn_cast_or_null<mlir::quantum::ValueSemanticsInstOp>(operation);

        if(!owner && owner2){
          wireset.emplace_back(temp1.front());
        }else if(owner && !owner2){
          wireset.emplace_back(temp1.back());
        }else {
          if(qllvm::matrix::ForwardOrReverse(temp1,qbitseq)){
            wireset.emplace_back(temp1.front());
            wireset.emplace_back(temp1.back());
            top_op.emplace_back(op);
          }else{
            wireset.emplace_back(temp1.back());
            wireset.emplace_back(temp1.front());
            top_op.emplace_back(op);
          }
        } 
      }
    }else{
      if(op.getNumResults() == 1){
        int inster = 0;
        auto index_one = temp1.front();
        auto it = wireset.begin();
        auto it2 = top_op.begin();
        for(;it != wireset.end();it++,it2++){
          idex_two.clear();
          idex_two.emplace_back(index_one);
          idex_two.emplace_back(*it);
          if(qllvm::matrix::ForwardOrReverse(idex_two,qbitseq)){
            wireset.insert(it,index_one);
            top_op.insert(it2,op);
            inster = 1;
            break;
          }
        }
        if(inster == 0){
          wireset.emplace_back(index_one);
          top_op.emplace_back(op);
        }
      }else if(op.getNumResults() == 2){
        int inster = 0;
        for(int j = 0;j < 2;j++){
          inster = 0;
          auto index_one = temp1[j];
          auto it = wireset.begin();
          auto it2 = top_op.begin();
          for(;it != wireset.end();it++,it2++){
            if(index_one == *it){
              inster = 1;
              break;
            }
            idex_two.clear();
            idex_two.emplace_back(index_one);
            idex_two.emplace_back(*it);
            if(qllvm::matrix::ForwardOrReverse(idex_two,qbitseq)){
              wireset.insert(it,index_one);
              top_op.insert(it2,op);
              inster = 1;
              break;
            }
          }
          if(inster == 0){
            wireset.emplace_back(index_one);
            top_op.emplace_back(op);
          }
        }
      }
    }
  }
}

void CommutativeCancellationPass::runOnOperation() {
  // std::cout << "CommutativeCancellationPass:  " << std::endl;
  // gen_qasm(getOperation(),"before.qasm");
  // getOperation().dump();
  qbit_seq.clear();
  hashTable.clear();
  std::vector<std::vector<mlir::quantum::ValueSemanticsInstOp>> cx;//["cx", "cy", "cz", "h", "y"]
  std::vector<std::vector<mlir::quantum::ValueSemanticsInstOp>> cy;
  std::vector<std::vector<mlir::quantum::ValueSemanticsInstOp>> cz;
  std::vector<std::vector<mlir::quantum::ValueSemanticsInstOp>> h;
  std::vector<std::vector<mlir::quantum::ValueSemanticsInstOp>> y;
  std::vector<std::vector<mlir::quantum::ValueSemanticsInstOp>> x;//["rx", "x"]
  std::vector<std::vector<mlir::quantum::ValueSemanticsInstOp>> z;//["p", "z", "u1", "rz", "t", "s"]

  std::vector<mlir::quantum::ValueSemanticsInstOp> top_op; 
  std::vector<std::vector<BiMap<mlir::quantum::ValueSemanticsInstOp, std::pair<std::string, int64_t>>>> commutation_set;
  std::vector<BiMap<mlir::quantum::ValueSemanticsInstOp, std::pair<std::string, int64_t>>> sub_commutation_vect;
  std::vector<mlir::quantum::ValueSemanticsInstOp> commutation_ops;
  std::vector<std::pair<std::string, int64_t>> idex_two;
  std::pair<std::string, int64_t> idex_one;
  int counts =0;

  getOperation().walk([&](mlir::quantum::QallocOp op) {
    qbit_seq.emplace(op.name().str(),counts++);
  });

  counts = 0;
  getOperation().walk([&](mlir::quantum::ValueSemanticsInstOp op) {
    idex_two.clear();
    mlir::OpBuilder rewriter(op);
    op.getOperation()->setAttr(llvm::StringRef("number"),mlir::IntegerAttr::get(rewriter.getI32Type(), counts));
    if(op.getNumResults() == 1){
      idex_one = OP::getbit_from_valueSemanticsInstOp(op);
      idex_two.emplace_back(idex_one);
    }else if(op.getNumResults() == 2){
      idex_two = OP::getbit_from_muti_valueSemanticsInstOp(op);
    }
    hashTable[counts] = idex_two;
    counts++;
  });

  topop.clear();
  circuit::getGateCountAndTopOp(before_gate_count, topop, getOperation());
  if(printCountAndDepth||syn){
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
  


  int index = 0;
  wire_set.clear();
  deal_topop(topop,hashTable,top_op,wire_set,qbit_seq);

  int i = 0;

  i = 0;
  std::pair<std::string, int64_t> wire;
  commutation_set.clear();
  for(auto &op : top_op){
    index = 0;
    commutation_ops.clear();
    commutation_ops.emplace_back(op);
    sub_commutation_vect.clear();
    BiMap<mlir::quantum::ValueSemanticsInstOp, std::pair<std::string, int64_t>> sub_commutation;
    wire = wire_set[i];
    sub_commutation.insert(op,wire);
    sub_commutation_vect.emplace_back(sub_commutation);
    findcommutativegate(op,commutation_ops,sub_commutation_vect,wire,index);
    commutation_set.emplace_back(sub_commutation_vect);
    i++;
  }

  CommutationGroupIndex groups;
  for (size_t w = 0; w < commutation_set.size(); ++w) {
    auto &wireGroups = groups[wire_set[w]];
    for (size_t g = 0; g < commutation_set[w].size(); ++g)
      for (auto op : commutation_set[w][g].getKeys())
        wireGroups.emplace(op.getOperation(), static_cast<int>(g));
  }

  i = 0;
  for(auto &vect:commutation_set){
    wire = wire_set[i];
    classification(groups,vect,h,y,z,x,cx,cy,cz,wire);
    i++;
  }

  std::string name;
  name = "rz";
  for(auto& vect : z){
    if(vect.size() > 1){
      replace_x_z(vect,name);
      for(auto &op1 : vect){
        op1->dropAllUses();
        op1.erase();
      }
    }
  }

  name = "rx";
  for(auto& vect : x){
    if(vect.size() > 1){
      replace_x_z(vect,name);
      for(auto &op1 : vect){
        op1->dropAllUses();
        op1.erase();
      }
    }
  }

  std::vector<std::string> gate = {"h","y","cx","cy","cz"};
  //delete cx_gate
  for(const auto& name1 : gate){
    if(name1 == "cx"){
      for(auto &vect: cx){
        delete_gate(vect,name1);
      }
    }else if(name1 == "cy"){
      for(auto &vect: cy){
        delete_gate(vect,name1);
      }
    }else if(name1 == "cz"){
      for(auto &vect: cz){
        delete_gate(vect,name1);
      }
    }else if(name1 == "h"){
      for(auto &vect: h){
        delete_gate(vect,name1);
      }
    }else if(name1 == "y"){
      for(auto &vect: y){
        delete_gate(vect,name1);
      }
    }
  }

  if(printCountAndDepth||syn){
    circuit::getGateCountAndTopOp(after_gate_count, topop, getOperation());
    int depth = 0;
    for (auto &op : topop) {
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
} // namespace qllvm
