
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
#include <tuple>
#include "utils/local_unitary.hpp"
#include "gen_qasm.hpp"
#include "utils/op.hpp"
#include <Eigen/Dense>
#include <eigen3/unsupported/Eigen/KroneckerProduct>
#include <unordered_map>
#include <tr1/unordered_map>
#include <stdexcept>
#include <unordered_set>
#include <cstring>
#include <cstdlib>
#include <chrono>

namespace qllvm {
void CommutativeCancellationPass::getDependentDialects(
    DialectRegistry &registry) const {
  registry.insert<LLVM::LLVMDialect>();
}

// Modified 2026-11-20: compact groups, preserving the old map key order.
// A wire's SSA chain visits an operation once; reverse maps were never queried.
using Wire = std::pair<std::string, int64_t>;
using WireTable = llvm::DenseMap<mlir::Operation *, std::vector<Wire>>;
class OperationGroup {
  std::vector<mlir::quantum::ValueSemanticsInstOp> operations;
public:
  void insert(mlir::quantum::ValueSemanticsInstOp op, const Wire &) {
    operations.push_back(op);
  }
  const std::vector<mlir::quantum::ValueSemanticsInstOp> &getKeys() const {
    // Modified 2026-11-22: findcommutativegate follows this wire's SSA
    // def-use chain. Insertion order is already causal circuit order;
    // sorting by operation address adds work and makes rewrites depend
    // on allocator layout instead of the program.
    return operations;
  }
};

// Modified 2026-11-20: memoize exact gate descriptors and operand-relative
// commutation checks within a read-only analysis. Nothing survives IR rewriting.
class CommutationAnalysis {
  const WireTable &hashTable;
  enum class Axis { None, X, Y, Z };
  struct Gate {
    Axis axis = Axis::None;
    Eigen::MatrixXcd matrix;
    bool valid = false;
    bool diagonal = false;
  };
  struct Identity { size_t gate, placement, family; };
  std::vector<Gate> gates;
  std::unordered_map<std::string, size_t> gateIds, placementIds, familyIds;
  llvm::DenseMap<mlir::Operation *, Identity> identities;
  std::unordered_map<std::string, bool> results;
  static constexpr size_t maxResults = 65536;

  template<class T> static void append(std::string &key, const T &value) {
    key.append(reinterpret_cast<const char *>(&value), sizeof(value));
  }
  Identity identity(mlir::quantum::ValueSemanticsInstOp op) {
    auto found = identities.find(op.getOperation());
    if (found != identities.end()) return found->second;
    std::string key = op.name().str();
    key.push_back('\0');
    // Exact floating-point bits: never round angles in a correctness cache.
    bool finite = true;
    try {
      for (size_t i = op.getNumResults(); i < op.getNumOperands(); ++i) {
        const double parameter = OP::tryGetConstAngle(op.getOperand(i));
        finite &= std::isfinite(parameter);
        append(key, parameter);
      }
    } catch (const std::exception &) { finite = false; }
    append(key, finite);
    const auto width = op.getNumResults();
    append(key, width);
    auto inserted = gateIds.emplace(key, gates.size());
    size_t gate = inserted.first->second;
    if (inserted.second) {
      Gate descriptor;
      if (finite && width > 0 && width <= 2 &&
          op.name() != "mz" && op.name() != "reset") {
        try {
          descriptor.matrix = matrix::getGateMat(op, 0);
          descriptor.valid = descriptor.matrix.allFinite() &&
              descriptor.matrix.rows() == (1 << width) &&
              descriptor.matrix.cols() == (1 << width);
          // Only exact diagonal matrices get the algebraic shortcut.
          descriptor.diagonal = descriptor.valid;
          for (int i = 0; i < descriptor.matrix.rows(); ++i)
            for (int j = 0; j < descriptor.matrix.cols(); ++j)
              if (i != j && descriptor.matrix(i, j) != std::complex<double>(0, 0))
                descriptor.diagonal = false;
        } catch (const std::exception &) {}
      }
      gates.push_back(std::move(descriptor));
    }
    // A family is an algebraic constraint, never a matrix-cache key. Keep
    // distinct angles in their exact descriptors for the numerical fallback.
    if (gates[gate].valid && width == 1) {
      auto name = op.name();
      if (name == "rx" || name == "x" || name == "sx") gates[gate].axis = Axis::X;
      else if (name == "ry" || name == "y") gates[gate].axis = Axis::Y;
      else if (name == "rz" || name == "z" || name == "p" || name == "u1" ||
               name == "t" || name == "tdg" || name == "s" || name == "sdg")
        gates[gate].axis = Axis::Z;
    }
    std::string placement;
    append(placement, gate);
    for (const auto &wire : hashTable.find(op.getOperation())->second) {
      placement.append(wire.first);
      placement.push_back('\0');
      append(placement, wire.second);
    }
    size_t id = placementIds.emplace(placement, placementIds.size()).first->second;
    std::string family;
    const auto axis = gates[gate].axis;
    append(family, axis);
    if (axis == Axis::None) append(family, id);
    else for (const auto &wire : hashTable.find(op.getOperation())->second) {
      family.append(wire.first); family.push_back('\0'); append(family, wire.second);
    }
    size_t familyId = familyIds.emplace(family, familyIds.size()).first->second;
    Identity value{gate, id, familyId};
    identities.insert({op.getOperation(), value});
    return value;
  }
public:
  explicit CommutationAnalysis(const WireTable &wires) : hashTable(wires) {}
  size_t familyProbes = 0, matrixChecks = 0, cacheHits = 0;
  size_t family(mlir::quantum::ValueSemanticsInstOp op) { return identity(op).family; }
  // Sufficient exact proofs for every member of the representative's family.
  // A failed proof is unknown, not noncommuting: special angles still fall back
  // to their own matrices. Do not infer an axis from a special-angle matrix.
  bool familyCommutes(mlir::quantum::ValueSemanticsInstOp op1,
                      mlir::quantum::ValueSemanticsInstOp op2) {
    ++familyProbes;
    if (op1->getBlock() != op2->getBlock()) return false;
    const auto id1 = identity(op1), id2 = identity(op2);
    const auto &a = gates[id1.gate], &b = gates[id2.gate];
    if (!a.valid || !b.valid) return false;
    const auto &wa = hashTable.find(op1.getOperation())->second;
    const auto &wb = hashTable.find(op2.getOperation())->second;
    if (wa.empty() || wb.empty()) return false;
    if (id1.family == id2.family) return true;
    bool disjoint = true;
    for (const auto &wire : wa)
      if (std::find(wb.begin(), wb.end(), wire) != wb.end()) disjoint = false;
    if (disjoint) return true;
    const bool diagonalA = a.axis == Axis::Z || (a.axis == Axis::None && a.diagonal);
    const bool diagonalB = b.axis == Axis::Z || (b.axis == Axis::None && b.diagonal);
    if (diagonalA && diagonalB) return true;
    // Same-axis controlled Paulis commute for a shared control or shared
    // target. Reversed/chain operands are deliberately excluded.
    if (wa.size() == 2 && wb.size() == 2 && op1.name() == op2.name() &&
        (op1.name() == "cx" || op1.name() == "cy") &&
        (wa[0] == wb[0] || wa[1] == wb[1])) return true;
    auto controlled = [](Axis axis, const auto &singleWire, auto gate, const auto &wires) {
      if (axis == Axis::None || wires.size() != 2) return false;
      if (gate == "cx" || gate == "cy") {
        if (singleWire == wires[0] && axis == Axis::Z) return true;
        if (singleWire == wires[1] &&
            axis == (gate == "cx" ? Axis::X : Axis::Y)) return true;
      }
      return gate == "cz" && axis == Axis::Z;
    };
    if (wa.size() == 1 && controlled(a.axis, wa[0], op2.name(), wb)) return true;
    if (wb.size() == 1 && controlled(b.axis, wb[0], op1.name(), wa)) return true;
    return false;
  }
  bool diagonal(mlir::quantum::ValueSemanticsInstOp op) {
    return gates[identity(op).gate].diagonal;
  }
  size_t placement(mlir::quantum::ValueSemanticsInstOp op) {
    return identity(op).placement;
  }
  bool valid(mlir::quantum::ValueSemanticsInstOp op) {
    return gates[identity(op).gate].valid;
  }
  bool commute(mlir::quantum::ValueSemanticsInstOp op1,
               mlir::quantum::ValueSemanticsInstOp op2) {
    if (op1->getBlock() != op2->getBlock()) return false;
    const auto id1 = identity(op1), id2 = identity(op2);
    const auto &first = gates[id1.gate], &second = gates[id2.gate];
    if (!first.valid || !second.valid) return false;
    const auto &wire1 = hashTable.find(op1.getOperation())->second;
    const auto &wire2 = hashTable.find(op2.getOperation())->second;
    if (wire1.empty() || wire2.empty()) return false;
    if (first.diagonal && second.diagonal) return true;
    if (id1.placement == id2.placement) return true;
    std::vector<std::pair<std::string, int64_t>> wires = wire1;
    for (const auto &wire : wire2)
      if (std::find(wires.begin(), wires.end(), wire) == wires.end())
        wires.push_back(wire);
    if (wires.size() == wire1.size() + wire2.size()) return true;
    auto axesFor = [&](const std::vector<std::pair<std::string, int64_t>> &operands) {
      std::vector<size_t> axes;
      for (const auto &wire : operands)
        axes.push_back(std::distance(wires.begin(), std::find(wires.begin(), wires.end(), wire)));
      return axes;
    };
    const auto axes1 = axesFor(wire1), axes2 = axesFor(wire2);
    std::string key;
    append(key, id1.gate); append(key, id2.gate);
    for (auto axis : axes2) append(key, axis);
    auto cached = results.find(key);
    if (cached != results.end()) { ++cacheHits; return cached->second; }
    ++matrixChecks;
    bool commutes = false;
    try {
      const auto a = matrix::embedLocalUnitary(first.matrix, axes1, wires.size());
      const auto b = matrix::embedLocalUnitary(second.matrix, axes2, wires.size());
      commutes = (a * b - b * a).norm() <= 1e-12;
    } catch (const std::exception &) {}
    if (results.size() < maxResults) results.emplace(std::move(key), commutes);
    return commutes;
  }
};

// Modified 2026-11-20: group obligations by generator and ordered support.
// A generic proof checks a whole family once. Unknown relations retain every
// exact representative, preserving the prior special-angle opportunities.
class CommutationFrontier {
  struct Family {
    std::vector<mlir::quantum::ValueSemanticsInstOp> members;
  };
  CommutationAnalysis &analysis;
  std::vector<Family> families;
  std::unordered_map<size_t, size_t> index;
  std::unordered_set<size_t> placements;
  mlir::Block *block = nullptr;
  bool allDiagonal = false;
public:
  explicit CommutationFrontier(CommutationAnalysis &analysis) : analysis(analysis) {}
  void reset(mlir::quantum::ValueSemanticsInstOp op) {
    families.clear(); index.clear(); placements.clear();
    block = op->getBlock(); allDiagonal = true; add(op);
  }
  void add(mlir::quantum::ValueSemanticsInstOp op) {
    allDiagonal = allDiagonal && analysis.diagonal(op);
    if (!placements.insert(analysis.placement(op)).second) return;
    auto inserted = index.emplace(analysis.family(op), families.size());
    if (inserted.second) families.emplace_back();
    families[inserted.first->second].members.push_back(op);
  }
  bool accepts(mlir::quantum::ValueSemanticsInstOp op) {
    if (op->getBlock() != block || !analysis.valid(op)) return false;
    if (placements.count(analysis.placement(op)) || (allDiagonal && analysis.diagonal(op)))
      return true;
    for (auto &family : families) {
      if (analysis.familyCommutes(op, family.members.front())) continue;
      for (auto member : family.members)
        if (!analysis.commute(op, member)) return false;
    }
    return true;
  }
};

void findcommutativegate(mlir::quantum::ValueSemanticsInstOp op,
std::vector<OperationGroup> &sub_commutation_vect, std::pair<std::string, int64_t> &wire, int &index, CommutationAnalysis &analysis, const WireTable &hashTable){
  bool does_commute;
  std::vector<std::pair<std::string, int64_t>> index_2bit;
  std::pair<std::string, int64_t> index_1bit;
  mlir::Value return_value;

  CommutationFrontier frontier(analysis);
  frontier.reset(op);
  bool cont = true;
  while(cont){
    does_commute = false;
    if(op.getNumResults() == 2){
      index_2bit = hashTable.find(op.getOperation())->second;
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
        does_commute = frontier.accepts(next_inst);
        if (does_commute) {
          sub_commutation_vect[index].insert(next_inst, wire);
          frontier.add(next_inst);
        } else {
          ++index;
          frontier.reset(next_inst);
          OperationGroup next;
          next.insert(next_inst, wire);
          sub_commutation_vect.emplace_back(std::move(next));
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

// Modified 2026-11-19: construct once per read-only classification phase.
// Looking through every group for every two-qubit gate is quadratic even
// without copying the groups. Retain the first occurrence, as the old scan did.
using CommutationIndex = std::map<std::pair<std::string, int64_t>,
                                  llvm::DenseMap<mlir::Operation *, int>>;
int find_controled_index(const CommutationIndex &groups,
    mlir::quantum::ValueSemanticsInstOp key,
    const std::vector<std::pair<std::string, int64_t>> &wire_site) {
  if (wire_site.empty()) return -1;
  auto wire = groups.find(wire_site.back());
  if (wire == groups.end()) return -1;
  auto op = wire->second.find(key.getOperation());
  return op == wire->second.end() ? -1 : op->second;
}

void deal_2gates(std::vector<mlir::quantum::ValueSemanticsInstOp> &vect,const CommutationIndex &commutation_set,
  std::vector<std::vector<mlir::quantum::ValueSemanticsInstOp>> &cx,std::pair<std::string, int64_t> &wire, const WireTable &hashTable){
  // Modified 2026-11-24: membership in this wire's commutation group
  // already proves that intervening operations commute. Bucket identical
  // ordered supports by the OTHER wire's group as well, rather than stopping
  // at the first commuting gate on a different target/control. Both wire
  // groups are required; shared control alone never licenses a rewrite.
  using Key = std::tuple<Wire, Wire, int>;
  std::map<Key, size_t> indices;
  std::vector<std::vector<mlir::quantum::ValueSemanticsInstOp>> candidates;
  for (auto op : vect) {
    const auto &support = hashTable.find(op.getOperation())->second;
    if (support.size() != 2 || support.front() != wire) continue;
    const int otherGroup = find_controled_index(commutation_set, op, support);
    if (otherGroup < 0) continue;
    Key key{support[0], support[1], otherGroup};
    auto entry = indices.emplace(key, candidates.size());
    if (entry.second) candidates.emplace_back();
    candidates[entry.first->second].push_back(op);
  }
  for (auto &candidate : candidates)
    if (candidate.size() > 1) cx.push_back(std::move(candidate));
}

void classification(const CommutationIndex &commutation_set,
std::vector<OperationGroup> &vect,
std::vector<std::vector<mlir::quantum::ValueSemanticsInstOp>> &h,std::vector<std::vector<mlir::quantum::ValueSemanticsInstOp>> &y,
std::vector<std::vector<mlir::quantum::ValueSemanticsInstOp>> &z,std::vector<std::vector<mlir::quantum::ValueSemanticsInstOp>> &x,
std::vector<std::vector<mlir::quantum::ValueSemanticsInstOp>> &cx,std::vector<std::vector<mlir::quantum::ValueSemanticsInstOp>>&cy,
std::vector<std::vector<mlir::quantum::ValueSemanticsInstOp>> &cz,std::pair<std::string, int64_t> wire, const WireTable &hashTable){

  std::vector<mlir::quantum::ValueSemanticsInstOp> cx_gate;//["cx", "cy", "cz", "h", "y"]
  std::vector<mlir::quantum::ValueSemanticsInstOp> cy_gate;
  std::vector<mlir::quantum::ValueSemanticsInstOp> cz_gate;
  std::vector<mlir::quantum::ValueSemanticsInstOp> h_gate;
  std::vector<mlir::quantum::ValueSemanticsInstOp> y_gate;
  std::vector<mlir::quantum::ValueSemanticsInstOp> z_rotation;//["p", "z", "u1", "rz", "t", "s"]
  std::vector<mlir::quantum::ValueSemanticsInstOp> x_rotation;//["rx", "x"]

  for (const auto &group : vect) {
    const auto &members = group.getKeys();
    h_gate.clear();
    y_gate.clear();
    z_rotation.clear();
    x_rotation.clear();
    cx_gate.clear();
    cy_gate.clear();
    cz_gate.clear();
    if(members.size() <= 1){
      continue;
    }
    for(auto tmp_op: members){
      if(tmp_op.name() == "h"){
        h_gate.emplace_back(tmp_op);
      }else if(tmp_op.name() == "y"){
        y_gate.emplace_back(tmp_op);
      }else if(tmp_op.name()=="p"||tmp_op.name()=="z"||tmp_op.name()=="u1"||
              tmp_op.name()=="rz"||tmp_op.name()=="t"||tmp_op.name()=="s"||tmp_op.name()=="tdg"||tmp_op.name()=="sdg"){
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
      deal_2gates(cx_gate,commutation_set,cx,wire,hashTable);
    }
    if(cy_gate.size() > 1){
      deal_2gates(cy_gate,commutation_set,cy,wire,hashTable);
    }
    if(cz_gate.size() > 1){
      deal_2gates(cz_gate,commutation_set,cz,wire,hashTable);
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

double total_angle(std::vector<mlir::quantum::ValueSemanticsInstOp> ops){
  double total_angle = 0,current_angle = 0;
  for(auto &op : ops){
    if(op.name() == "p"||op.name() == "u1"||op.name() == "rz"||op.name() == "rx"){
      current_angle = qllvm::OP::tryGetConstAngle(op.getOperand(1));
    }else if(op.name() == "z"||op.name() == "x"){
      current_angle = llvm::numbers::pi;
    }else if(op.name() == "t"){
      current_angle = llvm::numbers::pi/4;
    }else if(op.name() == "s"){
      current_angle = llvm::numbers::pi/2;
    }else if(op.name() == "tdg"){
      current_angle = -llvm::numbers::pi/4;
    }else if(op.name() == "sdg"){
      current_angle = -llvm::numbers::pi/2;
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
      optimization::markChanged();
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
        optimization::markChanged();
        op1.erase();
      }
    }
  }
}

void deal_topop(const std::vector<mlir::quantum::ValueSemanticsInstOp> &topop,const WireTable &hash_Table,
                std::vector<mlir::quantum::ValueSemanticsInstOp> &top_op,std::vector<std::pair<std::string, int64_t>> &wireset,
              const std::unordered_map<std::string, int> &qbitseq){
  std::vector<std::pair<std::string, int64_t>> idex_two;
  for(auto op : topop){
    const auto &temp1 = hash_Table.find(op.getOperation())->second;
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
  std::unordered_map<std::string, int> qbit_seq;
  std::vector<Wire> wire_set;
  WireTable hashTable;
  const bool trace = std::getenv("QLLVM_COMMUTATION_STATS");
  auto lastPhase = std::chrono::steady_clock::now();
  auto phase = [&](const char *name) {
    if (!trace) return;
    auto now = std::chrono::steady_clock::now();
    llvm::errs() << "[qllvm-commutation-phase] " << name << "="
                 << std::chrono::duration<double>(now - lastPhase).count() << "s\n";
    lastPhase = now;
  };
  // std::cout << "CommutativeCancellationPass:  " << std::endl;
  // gen_qasm(getOperation(),"before.qasm");
  // getOperation().dump();
  std::vector<std::vector<mlir::quantum::ValueSemanticsInstOp>> cx;//["cx", "cy", "cz", "h", "y"]
  std::vector<std::vector<mlir::quantum::ValueSemanticsInstOp>> cy;
  std::vector<std::vector<mlir::quantum::ValueSemanticsInstOp>> cz;
  std::vector<std::vector<mlir::quantum::ValueSemanticsInstOp>> h;
  std::vector<std::vector<mlir::quantum::ValueSemanticsInstOp>> y;
  std::vector<std::vector<mlir::quantum::ValueSemanticsInstOp>> x;//["rx", "x"]
  std::vector<std::vector<mlir::quantum::ValueSemanticsInstOp>> z;//["p", "z", "u1", "rz", "t", "s"]

  std::vector<mlir::quantum::ValueSemanticsInstOp> top_op; 
  std::vector<std::vector<OperationGroup>> commutation_set;
  std::vector<OperationGroup> sub_commutation_vect;
  std::vector<std::pair<std::string, int64_t>> idex_two;
  std::pair<std::string, int64_t> idex_one;
  int counts =0;

  getOperation().walk([&](mlir::quantum::QallocOp op) {
    qbit_seq.emplace(op.name().str(),counts++);
  });

  topop.clear();
  {
    OP::QuantumWireResolver wires;
    getOperation().walk([&](mlir::quantum::ValueSemanticsInstOp op) {
      idex_two.clear();
      if(op.getNumResults() == 1){
        idex_one = wires.resolve(op.qubits()[0]);
        idex_two.emplace_back(idex_one);
      }else if(op.getNumResults() == 2){
        for (auto value : op.qubits()) idex_two.push_back(wires.resolve(value));
      }
      hashTable.insert({op.getOperation(), idex_two});
      // Same roots as getGateCountAndTopOp, without resetting every depth
      // attribute when the default pipeline does not request statistics.
      for (size_t operand = 0; operand < op.getNumResults(); ++operand) {
        if (!dyn_cast_or_null<mlir::quantum::ValueSemanticsInstOp>(op.getOperand(operand).getDefiningOp())) {
          topop.push_back(op);
          if (operand == 0) break;
        }
      }
    });
  } // Discard the wire cache before any SSA rewrite.

  if (printCountAndDepth || syn)
    circuit::getGateCountAndTopOp(before_gate_count, topop, getOperation());
  phase("wire-index");
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
  {
  CommutationAnalysis analysis(hashTable);
  for(auto &op : top_op){
    index = 0;
    sub_commutation_vect.clear();
    OperationGroup sub_commutation;
    wire = wire_set[i];
    sub_commutation.insert(op,wire);
    sub_commutation_vect.emplace_back(std::move(sub_commutation));
    findcommutativegate(op,sub_commutation_vect,wire,index,analysis,hashTable);
    commutation_set.emplace_back(std::move(sub_commutation_vect));
    i++;
  }

  if (std::getenv("QLLVM_COMMUTATION_STATS"))
    llvm::errs() << "[qllvm-commutation] family_probes=" << analysis.familyProbes
                 << " matrix_checks=" << analysis.matrixChecks
                 << " cache_hits=" << analysis.cacheHits << "\n";
  } // Destroy all analysis caches before rewriting SSA.

  phase("grouping");
  CommutationIndex groupIndex;
  for (size_t wire = 0; wire < commutation_set.size(); ++wire) {
    if (groupIndex.count(wire_set[wire])) continue;
    auto &indexByOperation = groupIndex[wire_set[wire]];
    for (size_t group = 0; group < commutation_set[wire].size(); ++group)
      for (auto op : commutation_set[wire][group].getKeys())
        indexByOperation.insert({op.getOperation(), static_cast<int>(group)});
  }

  i = 0;

  for(auto &vect:commutation_set){
    wire = wire_set[i];
    classification(groupIndex,vect,h,y,z,x,cx,cy,cz,wire,hashTable);
    i++;
  }

  phase("classification");
  std::string name;
  name = "rz";
  for(auto& vect : z){
    if(vect.size() > 1){
      replace_x_z(vect,name);
      for(auto &op1 : vect){
        op1->dropAllUses();
        optimization::markChanged();
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
        optimization::markChanged();
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

  phase("rewrite");
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
