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
/*******************************************************************************
 * CircuitExtractor - extract circuit from QIR kernel
 *******************************************************************************/

#include "qllvm/passes/CircuitExtractor.hpp"

#include <llvm/IR/Constants.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <algorithm>
#include <unordered_map>
#include <vector>

namespace qllvm {
namespace sabre {

namespace {

using QubitMap = std::unordered_map<const llvm::Value*, int>;

const llvm::Value* traceToQubitValue(const llvm::Value* V) {
  if (!V) return nullptr;
  if (const llvm::BitCastInst* BC = llvm::dyn_cast<llvm::BitCastInst>(V))
    return traceToQubitValue(BC->getOperand(0));
  return V;
}

bool buildQubitMap(llvm::Function* F, QubitMap& qubitMap, int& totalQubits,
                  llvm::Value*& outAlloc) {
  totalQubits = 0;
  qubitMap.clear();
  outAlloc = nullptr;
  for (auto& BB : *F) {
    for (auto& I : BB) {
      llvm::CallInst* CI = llvm::dyn_cast<llvm::CallInst>(&I);
      if (!CI) continue;
      llvm::Function* Callee = CI->getCalledFunction();
      if (!Callee) continue;
      if (Callee->getName() != "__quantum__rt__qubit_allocate_array") continue;
      if (CI->getNumArgOperands() < 1) return false;
      if (!outAlloc) outAlloc = CI;
      const llvm::ConstantInt* CInt =
          llvm::dyn_cast<llvm::ConstantInt>(CI->getArgOperand(0));
      if (!CInt) return false;
      int N = (int)CInt->getZExtValue();
      int base = totalQubits;
      totalQubits += N;
      for (auto U : CI->users()) {
        llvm::CallInst* GEPCall = llvm::dyn_cast<llvm::CallInst>(U);
        if (!GEPCall || !GEPCall->getCalledFunction()) continue;
        if (GEPCall->getCalledFunction()->getName() !=
            "__quantum__rt__array_get_element_ptr_1d")
          continue;
        if (GEPCall->getNumArgOperands() < 2 || GEPCall->getArgOperand(0) != CI)
          continue;
        const llvm::ConstantInt* IdxC =
            llvm::dyn_cast<llvm::ConstantInt>(GEPCall->getArgOperand(1));
        if (!IdxC) continue;
        int idx = (int)IdxC->getZExtValue();
        int qidx = base + idx;
        for (auto GEPU : GEPCall->users()) {
          llvm::BitCastInst* BC = llvm::dyn_cast<llvm::BitCastInst>(GEPU);
          if (!BC) continue;
          for (auto BCU : BC->users()) {
            llvm::LoadInst* LI = llvm::dyn_cast<llvm::LoadInst>(BCU);
            if (!LI) continue;
            qubitMap[LI] = qidx;
          }
        }
      }
    }
  }
  return true;
}

int getQubitIndex(const llvm::Value* V, const QubitMap& qubitMap) {
  if (!V) return -1;
  auto it = qubitMap.find(V);
  if (it != qubitMap.end()) return it->second;
  return -1;
}

double getConstantDouble(const llvm::Value* V) {
  if (const llvm::ConstantFP* CF = llvm::dyn_cast<llvm::ConstantFP>(V))
    return CF->getValueAPF().convertToDouble();
  return 0.0;
}

bool is2QGate(GateKind k) {
  return k == GateKind::Cnot || k == GateKind::Cz || k == GateKind::Swap ||
         k == GateKind::Cp;
}

}  // namespace

bool CircuitExtractor::extract(llvm::Function* kernel, Circuit& out) {
  out.gates.clear();
  out.successors.clear();
  out.numQubits = 0;
  out.qubitArrayAlloc = nullptr;

  QubitMap qubitMap;
  if (!buildQubitMap(kernel, qubitMap, out.numQubits, out.qubitArrayAlloc))
    return false;

  std::vector<int> lastGateOnQubit(out.numQubits, -1);

  for (auto& BB : *kernel) {
    for (auto& I : BB) {
      llvm::CallInst* CI = llvm::dyn_cast<llvm::CallInst>(&I);
      if (!CI || !CI->getCalledFunction()) continue;
      llvm::StringRef name = CI->getCalledFunction()->getName();
      if (!name.startswith("__quantum__qis__")) continue;

      Gate g;
      g.callInst = CI;

      if (name == "__quantum__qis__h") {
        int q = getQubitIndex(CI->getArgOperand(0), qubitMap);
        if (q < 0) continue;
        g.kind = GateKind::H;
        g.qubits = {q};
        g.operands = {CI->getArgOperand(0)};
      } else if (name == "__quantum__qis__x" || name == "__quantum__qis__y" ||
                 name == "__quantum__qis__z") {
        int q = getQubitIndex(CI->getArgOperand(0), qubitMap);
        if (q < 0) continue;
        g.kind = (name.back() == 'x') ? GateKind::X :
                 (name.back() == 'y') ? GateKind::Y : GateKind::Z;
        g.qubits = {q};
        g.operands = {CI->getArgOperand(0)};
      } else if (name == "__quantum__qis__s") {
        int q = getQubitIndex(CI->getArgOperand(0), qubitMap);
        if (q < 0) continue;
        g.kind = GateKind::S;
        g.qubits = {q};
        g.operands = {CI->getArgOperand(0)};
      } else if (name == "__quantum__qis__sx") {
        int q = getQubitIndex(CI->getArgOperand(0), qubitMap);
        if (q < 0) continue;
        g.kind = GateKind::Sx;
        g.qubits = {q};
        g.operands = {CI->getArgOperand(0)};
      } else if (name == "__quantum__qis__X2P") {
        int q = getQubitIndex(CI->getArgOperand(0), qubitMap);
        if (q < 0) continue;
        g.kind = GateKind::X2P;
        g.qubits = {q};
        g.operands = {CI->getArgOperand(0)};
      } else if (name == "__quantum__qis__X2M") {
        int q = getQubitIndex(CI->getArgOperand(0), qubitMap);
        if (q < 0) continue;
        g.kind = GateKind::X2M;
        g.qubits = {q};
        g.operands = {CI->getArgOperand(0)};
      } else if (name == "__quantum__qis__Y2P") {
        int q = getQubitIndex(CI->getArgOperand(0), qubitMap);
        if (q < 0) continue;
        g.kind = GateKind::Y2P;
        g.qubits = {q};
        g.operands = {CI->getArgOperand(0)};
      } else if (name == "__quantum__qis__sdg") {
        int q = getQubitIndex(CI->getArgOperand(0), qubitMap);
        if (q < 0) continue;
        g.kind = GateKind::Sdg;
        g.qubits = {q};
        g.operands = {CI->getArgOperand(0)};
      } else if (name == "__quantum__qis__t") {
        int q = getQubitIndex(CI->getArgOperand(0), qubitMap);
        if (q < 0) continue;
        g.kind = GateKind::T;
        g.qubits = {q};
        g.operands = {CI->getArgOperand(0)};
      } else if (name == "__quantum__qis__tdg") {
        int q = getQubitIndex(CI->getArgOperand(0), qubitMap);
        if (q < 0) continue;
        g.kind = GateKind::Tdg;
        g.qubits = {q};
        g.operands = {CI->getArgOperand(0)};
      } else if (name == "__quantum__qis__rx") {
        int q = getQubitIndex(CI->getArgOperand(1), qubitMap);
        if (q < 0) continue;
        g.kind = GateKind::Rx;
        g.qubits = {q};
        g.params = {getConstantDouble(CI->getArgOperand(0))};
        g.operands = {CI->getArgOperand(1)};
      } else if (name == "__quantum__qis__ry") {
        int q = getQubitIndex(CI->getArgOperand(1), qubitMap);
        if (q < 0) continue;
        g.kind = GateKind::Ry;
        g.qubits = {q};
        g.params = {getConstantDouble(CI->getArgOperand(0))};
        g.operands = {CI->getArgOperand(1)};
      } else if (name == "__quantum__qis__rz") {
        int q = getQubitIndex(CI->getArgOperand(1), qubitMap);
        if (q < 0) continue;
        g.kind = GateKind::Rz;
        g.qubits = {q};
        g.params = {getConstantDouble(CI->getArgOperand(0))};
        g.operands = {CI->getArgOperand(1)};
      } else if (name == "__quantum__qis__p") {
        int q = getQubitIndex(CI->getArgOperand(1), qubitMap);
        if (q < 0) continue;
        g.kind = GateKind::P;
        g.qubits = {q};
        g.params = {getConstantDouble(CI->getArgOperand(0))};
        g.operands = {CI->getArgOperand(1)};
      } else if (name == "__quantum__qis__u3") {
        int q = getQubitIndex(CI->getArgOperand(3), qubitMap);
        if (q < 0) continue;
        g.kind = GateKind::U3;
        g.qubits = {q};
        g.params = {getConstantDouble(CI->getArgOperand(0)),
                   getConstantDouble(CI->getArgOperand(1)),
                   getConstantDouble(CI->getArgOperand(2))};
        g.operands = {CI->getArgOperand(3)};
      } else if (name == "__quantum__qis__cnot") {
        int q0 = getQubitIndex(CI->getArgOperand(0), qubitMap);
        int q1 = getQubitIndex(CI->getArgOperand(1), qubitMap);
        if (q0 < 0 || q1 < 0) continue;
        g.kind = GateKind::Cnot;
        g.qubits = {q0, q1};
        g.operands = {CI->getArgOperand(0), CI->getArgOperand(1)};
      } else if (name == "__quantum__qis__cz") {
        int q0 = getQubitIndex(CI->getArgOperand(0), qubitMap);
        int q1 = getQubitIndex(CI->getArgOperand(1), qubitMap);
        if (q0 < 0 || q1 < 0) continue;
        g.kind = GateKind::Cz;
        g.qubits = {q0, q1};
        g.operands = {CI->getArgOperand(0), CI->getArgOperand(1)};
      } else if (name == "__quantum__qis__swap") {
        int q0 = getQubitIndex(CI->getArgOperand(0), qubitMap);
        int q1 = getQubitIndex(CI->getArgOperand(1), qubitMap);
        if (q0 < 0 || q1 < 0) continue;
        g.kind = GateKind::Swap;
        g.qubits = {q0, q1};
        g.operands = {CI->getArgOperand(0), CI->getArgOperand(1)};
      } else if (name == "__quantum__qis__cp" || name == "__quantum__qis__cphase") {
        int q0 = getQubitIndex(CI->getArgOperand(1), qubitMap);
        int q1 = getQubitIndex(CI->getArgOperand(2), qubitMap);
        if (q0 < 0 || q1 < 0) continue;
        g.kind = GateKind::Cp;
        g.qubits = {q0, q1};
        g.params = {getConstantDouble(CI->getArgOperand(0))};
        g.operands = {CI->getArgOperand(1), CI->getArgOperand(2)};
      } else if (name == "__quantum__qis__mz" || name == "__quantum__qis__reset") {
        int q = getQubitIndex(CI->getArgOperand(0), qubitMap);
        if (q < 0) continue;
        g.kind = (name == "__quantum__qis__mz") ? GateKind::Mz : GateKind::Reset;
        g.qubits = {q};
        g.operands = {CI->getArgOperand(0)};
      } else {
        continue;
      }

      int gi = (int)out.gates.size();
      for (int q : g.qubits) {
        if (lastGateOnQubit[q] >= 0) {
          out.successors[lastGateOnQubit[q]].push_back(gi);
        }
        lastGateOnQubit[q] = gi;
      }
      out.gates.push_back(std::move(g));
      out.successors.resize(out.gates.size());
    }
  }

  // SabrePass pads qubit_allocate_array to numPhys so each physical node can
  // host a logical wire during routing. numQubits must match that width or
  // invLayout has holes and SabreSwap cannot insert swaps on a sparse graph.
  int maxQ = -1;
  for (const Gate& g : out.gates) {
    for (int q : g.qubits) maxQ = std::max(maxQ, q);
  }
  int active = maxQ >= 0 ? maxQ + 1 : 0;
  int allocN = active;
  if (out.qubitArrayAlloc) {
    if (auto* ac = llvm::dyn_cast<llvm::CallInst>(out.qubitArrayAlloc)) {
      if (ac->getNumArgOperands() >= 1)
        if (auto* c = llvm::dyn_cast<llvm::ConstantInt>(ac->getArgOperand(0)))
          allocN = (int)c->getZExtValue();
    }
  }
  out.numQubits = std::max(active, allocN);
  return true;
}

}  // namespace sabre
}  // namespace qllvm
