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
 * QIR to OpenQASM translator - Part of qllvm backend module
 *******************************************************************************/

#include "QirToQasm.hpp"

#include <llvm/Analysis/ConstantFolding.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/raw_ostream.h>

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/SmallVector.h"

using namespace llvm;

namespace qllvm {

namespace {

// Map Value* (Qubit* SSA) -> global qubit index
using QubitMap = std::unordered_map<const Value*, int>;

// Find kernel function: __internal_mlir_<name> or first __internal_mlir_*
Function* findKernelFunction(Module* M, const std::string& kernelName) {
  Function* found = nullptr;
  for (auto& F : *M) {
    if (F.isDeclaration()) continue;
    StringRef name = F.getName();
    if (name.startswith("__internal_mlir_")) {
      if (!kernelName.empty()) {
        std::string expected = "__internal_mlir_" + kernelName;
        if (name == expected) return &F;
      }
      if (!found) found = &F;
    }
  }
  return found;
}

// Resolve qubit index from Value* (may be through bitcast)
const Value* traceToQubitValue(const Value* V) {
  if (!V) return nullptr;
  // BitCast: i8* -> Qubit**, the load uses the result
  if (const BitCastInst* BC = dyn_cast<BitCastInst>(V))
    return traceToQubitValue(BC->getOperand(0));
  return V;
}

// Build qubit map from kernel function
bool buildQubitMap(Function* F, QubitMap& qubitMap, int& totalQubits) {
  totalQubits = 0;
  qubitMap.clear();

  for (auto& BB : *F) {
    for (auto& I : BB) {
      CallInst* CI = dyn_cast<CallInst>(&I);
      if (!CI) continue;

      Function* Callee = CI->getCalledFunction();
      if (!Callee) continue;

      StringRef name = Callee->getName();

      if (name == "__quantum__rt__qubit_allocate_array") {
        if (CI->getNumArgOperands() < 1) return false;
        const Value* NVal = CI->getArgOperand(0);
        const ConstantInt* CInt = dyn_cast<ConstantInt>(NVal);
        if (!CInt) return false;
        int N = (int)CInt->getZExtValue();
        // Array %array; qubits will be 0..N-1 for this array
        // We need to map: get_element_ptr_1d(%array, idx) -> totalQubits + idx
        int base = totalQubits;
        totalQubits += N;

        // Scan forward to find get_element_ptr_1d uses of this array
        for (auto U : CI->users()) {
          CallInst* GEPCall = dyn_cast<CallInst>(U);
          if (!GEPCall || !GEPCall->getCalledFunction()) continue;
          if (GEPCall->getCalledFunction()->getName() !=
              "__quantum__rt__array_get_element_ptr_1d")
            continue;
          if (GEPCall->getNumArgOperands() < 2) continue;
          if (GEPCall->getArgOperand(0) != CI) continue;
          const Value* IdxVal = GEPCall->getArgOperand(1);
          const ConstantInt* IdxC = dyn_cast<ConstantInt>(IdxVal);
          if (!IdxC) continue;
          int idx = (int)IdxC->getZExtValue();
          int qidx = base + idx;

          qubitMap[GEPCall] = qidx;
          for (auto GEPU : GEPCall->users()) {
            if (BitCastInst* BC = dyn_cast<BitCastInst>(GEPU)) {
              qubitMap[BC] = qidx;
              for (auto BCU : BC->users()) {
                if (LoadInst* LI = dyn_cast<LoadInst>(BCU))
                  qubitMap[LI] = qidx;
              }
            } else if (LoadInst* LI = dyn_cast<LoadInst>(GEPU)) {
              qubitMap[LI] = qidx;
            }
          }
        }
      }
    }
  }
  return true;
}

// Get qubit index from Value*, return -1 if not found
int getQubitIndex(const Value* V, const QubitMap& qubitMap) {
  if (!V) return -1;
  auto it = qubitMap.find(V);
  if (it != qubitMap.end()) return it->second;
  // Handle phi/select by checking operand (simplified)
  return -1;
}

// Extract double from ConstantFP or return 0
double getConstantDouble(const Value* V) {
  if (const ConstantFP* CF = dyn_cast<ConstantFP>(V))
    return CF->getValueAPF().convertToDouble();
  return 0.0;
}

/// Memref lowering uses insertvalue/extractvalue chains ending at alloca i1[N].
static const AllocaInst *findAllocaInInsertValueChain(Value *V) {
  SmallVector<Value *, 8> worklist;
  SmallPtrSet<Value *, 32> seen;
  worklist.push_back(V);
  while (!worklist.empty()) {
    Value *W = worklist.pop_back_val();
    if (!seen.insert(W).second)
      continue;
    if (auto *AI = dyn_cast<AllocaInst>(W))
      return AI;
    if (auto *IV = dyn_cast<InsertValueInst>(W)) {
      for (Use &U : IV->operands())
        worklist.push_back(U.get());
      continue;
    }
    if (auto *EV = dyn_cast<ExtractValueInst>(W)) {
      worklist.push_back(EV->getAggregateOperand());
      continue;
    }
  }
  return nullptr;
}

static const AllocaInst *tracePtrToClassicalAlloca(Value *V) {
  for (unsigned n = 0; n < 256; ++n) {
    if (auto *AI = dyn_cast<AllocaInst>(V))
      return AI;
    if (auto *GEP = dyn_cast<GetElementPtrInst>(V)) {
      V = GEP->getPointerOperand();
      continue;
    }
    if (auto *BC = dyn_cast<BitCastInst>(V)) {
      V = BC->getOperand(0);
      continue;
    }
    if (isa<InsertValueInst>(V) || isa<ExtractValueInst>(V))
      return findAllocaInInsertValueChain(V);
    if (auto *Ld = dyn_cast<LoadInst>(V)) {
      V = Ld->getPointerOperand();
      continue;
    }
    return nullptr;
  }
  return nullptr;
}

/// OpenQASM 2 packs creg bits into an integer for `if(creg==k)`; LLVM uses
/// add/zext/shl over loads — trace back to one classical alloca.
static const AllocaInst *traceClassicalPackedCmpValue(Value *V) {
  if (!V)
    return nullptr;
  if (auto *Ld = dyn_cast<LoadInst>(V))
    return tracePtrToClassicalAlloca(Ld->getPointerOperand());
  if (auto *Z = dyn_cast<ZExtInst>(V))
    return traceClassicalPackedCmpValue(Z->getOperand(0));
  if (auto *BO = dyn_cast<BinaryOperator>(V)) {
    if (BO->getOpcode() == Instruction::Shl)
      return traceClassicalPackedCmpValue(BO->getOperand(0));
    if (BO->getOpcode() == Instruction::Add) {
      const AllocaInst *A0 = traceClassicalPackedCmpValue(BO->getOperand(0));
      const AllocaInst *A1 = traceClassicalPackedCmpValue(BO->getOperand(1));
      if (A0 && A1 && A0 != A1)
        return nullptr;
      return A0 ? A0 : A1;
    }
  }
  return nullptr;
}

struct ClassicalLayout {
  std::vector<const AllocaInst *> order;
  std::vector<unsigned> bitWidth;
  std::unordered_map<const AllocaInst *, unsigned> indexOf;

  bool empty() const { return order.empty(); }

  unsigned getIndex(const AllocaInst *A) const {
    auto it = indexOf.find(A);
    if (it == indexOf.end())
      return 0;
    return it->second;
  }

  std::string name(unsigned regIdx) const {
    if (order.size() == 1)
      return "c";
    if (order.size() == 2 && bitWidth.size() == 2 && bitWidth[0] == 3 &&
        bitWidth[1] == 2)
      return regIdx == 0 ? "c" : "syn";
    bool allOneBit = !bitWidth.empty() &&
                     std::all_of(bitWidth.begin(), bitWidth.end(),
                                 [](unsigned w) { return w == 1; });
    if (allOneBit && order.size() > 1)
      return "c" + std::to_string(regIdx);
    return "r" + std::to_string(regIdx);
  }
};

/// Memref alloca uses `alloca i1, i64 mul(ptrtoint, i64 N)` or
/// `i64 ptrtoint (i1* getelementptr ...)` — fold to a concrete element count.
static std::optional<unsigned> getI1AllocaNumBits(const AllocaInst *AI,
                                                  const DataLayout &DL) {
  if (!AI->getAllocatedType()->isIntegerTy(1))
    return std::nullopt;
  if (!AI->isArrayAllocation())
    return std::nullopt;
  const Value *AS = AI->getArraySize();
  if (auto *CI = dyn_cast<ConstantInt>(AS))
    return (unsigned)CI->getZExtValue();
  if (auto *C = dyn_cast<Constant>(AS)) {
    Constant *Folded = ConstantFoldConstant(C, DL, nullptr);
    if (auto *CI2 = dyn_cast<ConstantInt>(Folded)) {
      uint64_t v = CI2->getZExtValue();
      if (v > 0 && v < 65536)
        return (unsigned)v;
    }
  }
  if (auto *CE = dyn_cast<ConstantExpr>(AS)) {
    if (CE->getOpcode() == Instruction::Mul && CE->getNumOperands() >= 2) {
      unsigned best = 0;
      for (unsigned i = 0; i < CE->getNumOperands(); ++i) {
        if (auto *C = dyn_cast<ConstantInt>(CE->getOperand(i))) {
          uint64_t v = C->getZExtValue();
          if (v > 0 && v < 65536)
            best = std::max(best, (unsigned)v);
        }
      }
      if (best > 0)
        return best;
    }
  }
  return std::nullopt;
}

static ClassicalLayout buildClassicalLayout(Function *F) {
  const DataLayout &DL = F->getParent()->getDataLayout();
  ClassicalLayout L;
  for (Instruction &I : F->getEntryBlock()) {
    auto *AI = dyn_cast<AllocaInst>(&I);
    if (!AI)
      continue;
    auto nBitsOpt = getI1AllocaNumBits(AI, DL);
    if (!nBitsOpt)
      continue;
    unsigned nBits = *nBitsOpt;
    L.indexOf[AI] = (unsigned)L.order.size();
    L.order.push_back(AI);
    L.bitWidth.push_back(nBits);
  }
  return L;
}

static StoreInst *findStoreForMzResult(CallInst *Mz) {
  for (User *U : Mz->users()) {
    auto *BC = dyn_cast<BitCastInst>(U);
    if (!BC)
      continue;
    for (User *U2 : BC->users()) {
      auto *Ld = dyn_cast<LoadInst>(U2);
      if (!Ld)
        continue;
      for (User *U3 : Ld->users()) {
        if (auto *St = dyn_cast<StoreInst>(U3)) {
          if (St->getValueOperand() == Ld)
            return St;
        }
      }
    }
  }
  return nullptr;
}

static bool getMeasureClassicalTarget(StoreInst *St, const ClassicalLayout &L,
                                      unsigned &regIdx, unsigned &bitIdx) {
  Value *Ptr = St->getPointerOperand();
  bitIdx = 0;
  if (auto *GEP = dyn_cast<GetElementPtrInst>(Ptr)) {
    if (GEP->getNumIndices() >= 1) {
      if (auto *CI = dyn_cast<ConstantInt>(GEP->getOperand(1)))
        bitIdx = (unsigned)CI->getZExtValue();
    }
    Ptr = GEP->getPointerOperand();
  }
  const AllocaInst *A = tracePtrToClassicalAlloca(Ptr);
  if (!A || !L.indexOf.count(A))
    return false;
  regIdx = L.indexOf.find(A)->second;
  return true;
}

/// Match OpenQASM-style classical control: same BB ends with
/// `icmp eq <intty> %v, K` + `br i1 %cmp, label %Then, label %Merge` where
/// `%Then` is only quantum ops and terminates with `br label %Merge`.
static bool matchClassicalIfThenMerge(BasicBlock* BB, uint64_t& KOut,
                                      BasicBlock** ThenOut,
                                      BasicBlock** MergeOut,
                                      const AllocaInst** RegAllocaOut) {
  auto* BI = dyn_cast<BranchInst>(BB->getTerminator());
  if (!BI || !BI->isConditional()) return false;

  auto* IC = dyn_cast<ICmpInst>(BI->getCondition());
  if (!IC || IC->getPredicate() != CmpInst::ICMP_EQ) return false;

  Value* LHS = IC->getOperand(0);
  Value* RHS = IC->getOperand(1);
  ConstantInt* CK = dyn_cast<ConstantInt>(RHS);
  Value* CmpVal = LHS;
  if (!CK) {
    CK = dyn_cast<ConstantInt>(LHS);
    if (!CK) return false;
    CmpVal = RHS;
  }

  const AllocaInst *RegA = traceClassicalPackedCmpValue(CmpVal);
  if (!RegA)
    return false;
  *RegAllocaOut = RegA;

  BasicBlock* Then = BI->getSuccessor(0);
  BasicBlock* Merge = BI->getSuccessor(1);

  auto* TBr = dyn_cast<BranchInst>(Then->getTerminator());
  if (!TBr || !TBr->isUnconditional()) return false;
  if (TBr->getNumSuccessors() != 1 || TBr->getSuccessor(0) != Merge)
    return false;

  // Then may contain GEP/load/bitcast before __quantum__qis__ calls; require at
  // least one quantum intrinsic.
  bool sawQuantum = false;
  for (Instruction& I : *Then) {
    if (&I == Then->getTerminator()) break;
    auto* CI = dyn_cast<CallInst>(&I);
    if (!CI) continue;
    Function* F = CI->getCalledFunction();
    if (F && F->getName().startswith("__quantum__qis__")) sawQuantum = true;
  }
  if (!sawQuantum) return false;

  // Unsigned integer value of the packed classical register (matches OQ2).
  KOut = CK->getValue().getZExtValue();
  *ThenOut = Then;
  *MergeOut = Merge;
  return true;
}

// Emit gate/measure to string stream. If \p ifPrefix is non-empty, emit OpenQASM 2
// classical conditioning: `if(creg==k) <gate>;`
bool emitInstruction(CallInst* CI, const QubitMap& qubitMap,
                     std::ostream& out, int& measureCount,
                     const ClassicalLayout* classicLayout,
                     const std::string& ifPrefix = {}) {
  Function* Callee = CI->getCalledFunction();
  if (!Callee) return true;  // skip indirect calls
  StringRef name = Callee->getName();

  if (!name.startswith("__quantum__qis__")) return true;

  auto prependIf = [&]() {
    if (!ifPrefix.empty()) out << ifPrefix;
  };
  auto emit1q = [&](const std::string& gate, int q) {
    if (q >= 0) {
      prependIf();
      out << gate << " q[" << q << "];\n";
    }
  };
  auto emit1qParam = [&](const std::string& gate, double theta, int q) {
    if (q >= 0) {
      prependIf();
      out << gate << "(" << std::setprecision(17) << theta << ") q[" << q
          << "];\n";
    }
  };
  auto emit2q = [&](const std::string& gate, int q0, int q1) {
    if (q0 >= 0 && q1 >= 0) {
      prependIf();
      out << gate << " q[" << q0 << "], q[" << q1 << "];\n";
    }
  };
  auto emit2qParam = [&](const std::string& gate, double theta, int q0,
                         int q1) {
    if (q0 >= 0 && q1 >= 0) {
      prependIf();
      out << gate << "(" << std::setprecision(17) << theta << ") q[" << q0
          << "], q[" << q1 << "];\n";
    }
  };
  auto emit3qParam = [&](double t, double p, double l, int q) {
    if (q >= 0) {
      prependIf();
      out << "u3(" << std::setprecision(17) << t << "," << p << "," << l
          << ") q[" << q << "];\n";
    }
  };

  if (name == "__quantum__qis__h") {
    int q = getQubitIndex(CI->getArgOperand(0), qubitMap);
    emit1q("h", q);
    return q >= 0;
  }
  if (name == "__quantum__qis__x" || name == "__quantum__qis__y" ||
      name == "__quantum__qis__z") {
    int q = getQubitIndex(CI->getArgOperand(0), qubitMap);
    char c = name.back();
    emit1q(std::string(1, c), q);
    return q >= 0;
  }
  if (name == "__quantum__qis__s") {
    emit1q("s", getQubitIndex(CI->getArgOperand(0), qubitMap));
    return true;
  }
  if (name == "__quantum__qis__sx") {
    emit1q("sx", getQubitIndex(CI->getArgOperand(0), qubitMap));
    return true;
  }
  if (name == "__quantum__qis__sdg") {
    emit1q("sdg", getQubitIndex(CI->getArgOperand(0), qubitMap));
    return true;
  }
  if (name == "__quantum__qis__t") {
    emit1q("t", getQubitIndex(CI->getArgOperand(0), qubitMap));
    return true;
  }
  if (name == "__quantum__qis__tdg") {
    emit1q("tdg", getQubitIndex(CI->getArgOperand(0), qubitMap));
    return true;
  }
  if (name == "__quantum__qis__rx") {
    double theta = getConstantDouble(CI->getArgOperand(0));
    int q = getQubitIndex(CI->getArgOperand(1), qubitMap);
    emit1qParam("rx", theta, q);
    return q >= 0;
  }
  if (name == "__quantum__qis__ry") {
    double theta = getConstantDouble(CI->getArgOperand(0));
    int q = getQubitIndex(CI->getArgOperand(1), qubitMap);
    emit1qParam("ry", theta, q);
    return q >= 0;
  }
  if (name == "__quantum__qis__rz") {
    double theta = getConstantDouble(CI->getArgOperand(0));
    int q = getQubitIndex(CI->getArgOperand(1), qubitMap);
    emit1qParam("rz", theta, q);
    return q >= 0;
  }
  if (name == "__quantum__qis__p") {
    double theta = getConstantDouble(CI->getArgOperand(0));
    int q = getQubitIndex(CI->getArgOperand(1), qubitMap);
    emit1qParam("p", theta, q);
    return q >= 0;
  }
  if (name == "__quantum__qis__u3") {
    double theta = getConstantDouble(CI->getArgOperand(0));
    double phi = getConstantDouble(CI->getArgOperand(1));
    double lambda = getConstantDouble(CI->getArgOperand(2));
    int q = getQubitIndex(CI->getArgOperand(3), qubitMap);
    emit3qParam(theta, phi, lambda, q);
    return q >= 0;
  }
  if (name == "__quantum__qis__su2") {
    double theta = getConstantDouble(CI->getArgOperand(0));
    double lambda = getConstantDouble(CI->getArgOperand(1));
    double phi = getConstantDouble(CI->getArgOperand(2));
    int q = getQubitIndex(CI->getArgOperand(3), qubitMap);
    emit3qParam(theta, phi, lambda, q);
    return q >= 0;
  }
  if (name == "__quantum__qis__cnot") {
    int q0 = getQubitIndex(CI->getArgOperand(0), qubitMap);
    int q1 = getQubitIndex(CI->getArgOperand(1), qubitMap);
    emit2q("cx", q0, q1);
    return q0 >= 0 && q1 >= 0;
  }
  if (name == "__quantum__qis__cz") {
    int q0 = getQubitIndex(CI->getArgOperand(0), qubitMap);
    int q1 = getQubitIndex(CI->getArgOperand(1), qubitMap);
    emit2q("cz", q0, q1);
    return q0 >= 0 && q1 >= 0;
  }
  if (name == "__quantum__qis__swap") {
    int q0 = getQubitIndex(CI->getArgOperand(0), qubitMap);
    int q1 = getQubitIndex(CI->getArgOperand(1), qubitMap);
    emit2q("swap", q0, q1);
    return q0 >= 0 && q1 >= 0;
  }
  if (name == "__quantum__qis__cp" || name == "__quantum__qis__cphase") {
    double theta = getConstantDouble(CI->getArgOperand(0));
    int q0 = getQubitIndex(CI->getArgOperand(1), qubitMap);
    int q1 = getQubitIndex(CI->getArgOperand(2), qubitMap);
    emit2qParam("cp", theta, q0, q1);
    return q0 >= 0 && q1 >= 0;
  }
  if (name == "__quantum__qis__mz") {
    int q = getQubitIndex(CI->getArgOperand(0), qubitMap);
    if (q >= 0) {
      prependIf();
      if (classicLayout && !classicLayout->empty()) {
        if (StoreInst *St = findStoreForMzResult(CI)) {
          unsigned ridx = 0, bidx = 0;
          if (getMeasureClassicalTarget(St, *classicLayout, ridx, bidx)) {
            out << "measure q[" << q << "] -> " << classicLayout->name(ridx) << "["
                << bidx << "];\n";
            return true;
          }
        }
      }
      out << "measure q[" << q << "] -> c[" << measureCount << "];\n";
      measureCount++;
    }
    return q >= 0;
  }
  if (name == "__quantum__qis__reset") {
    int q = getQubitIndex(CI->getArgOperand(0), qubitMap);
    emit1q("reset", q);
    return q >= 0;
  }

  return true;  // unknown gate, skip
}

}  // namespace

std::string QirToQasmTranslator::translate(llvm::Module* module,
                                           const std::string& kernelName) {
  if (!module) return "";

  Function* Kernel = findKernelFunction(module, kernelName);
  if (!Kernel) return "";

  QubitMap qubitMap;
  int totalQubits = 0;
  if (!buildQubitMap(Kernel, qubitMap, totalQubits)) return "";

  ClassicalLayout classicLayout = buildClassicalLayout(Kernel);

  int measureCount = 0;
  std::ostringstream out;

  out << "OPENQASM 2.0;\n";
  out << "include \"qelib1.inc\";\n";
  out << "qreg q[" << totalQubits << "];\n";

  if (classicLayout.empty()) {
    for (auto& BB : *Kernel) {
      for (auto& I : BB) {
        CallInst* CI = dyn_cast<CallInst>(&I);
        if (!CI || !CI->getCalledFunction()) continue;
        if (CI->getCalledFunction()->getName() == "__quantum__qis__mz")
          measureCount++;
      }
    }
    out << "creg c[" << measureCount << "];\n";
  } else {
    for (unsigned i = 0; i < classicLayout.order.size(); ++i)
      out << "creg " << classicLayout.name(i) << "[" << classicLayout.bitWidth[i]
          << "];\n";
  }

  struct ClassicalIfPatch {
    uint64_t k;
    BasicBlock *thenBB;
    const AllocaInst *regAlloca;
  };
  std::unordered_map<BasicBlock *, ClassicalIfPatch> classicalIfTests;
  std::unordered_set<Instruction *> skipCallsInThen;
  for (BasicBlock &BB : *Kernel) {
    uint64_t K = 0;
    BasicBlock *ThenBB = nullptr;
    BasicBlock *MergeBB = nullptr;
    const AllocaInst *RegAlloca = nullptr;
    if (matchClassicalIfThenMerge(&BB, K, &ThenBB, &MergeBB, &RegAlloca)) {
      classicalIfTests[&BB] = {K, ThenBB, RegAlloca};
      for (Instruction &I : *ThenBB) {
        if (auto *CI = dyn_cast<CallInst>(&I)) {
          Function *F = CI->getCalledFunction();
          if (F && F->getName().startswith("__quantum__qis__"))
            skipCallsInThen.insert(&I);
        }
      }
    }
  }

  const ClassicalLayout *layoutPtr =
      classicLayout.empty() ? nullptr : &classicLayout;

  measureCount = 0;
  for (BasicBlock &BB : *Kernel) {
    for (Instruction &I : BB) {
      auto *CI = dyn_cast<CallInst>(&I);
      if (!CI)
        continue;
      if (skipCallsInThen.count(&I))
        continue;
      emitInstruction(CI, qubitMap, out, measureCount, layoutPtr, "");
    }
    auto ifIt = classicalIfTests.find(&BB);
    if (ifIt != classicalIfTests.end()) {
      uint64_t K = ifIt->second.k;
      BasicBlock *ThenBB = ifIt->second.thenBB;
      const AllocaInst *RA = ifIt->second.regAlloca;
      std::string regName = "c";
      if (layoutPtr && RA && classicLayout.indexOf.count(RA))
        regName = classicLayout.name(classicLayout.getIndex(RA));
      std::string prefix = "if(" + regName + "==" + std::to_string(K) + ") ";
      for (Instruction &I : *ThenBB) {
        if (auto *CI = dyn_cast<CallInst>(&I)) {
          Function *F = CI->getCalledFunction();
          if (F && F->getName().startswith("__quantum__qis__"))
            emitInstruction(CI, qubitMap, out, measureCount, layoutPtr,
                            prefix);
        }
      }
    }
  }

  return out.str();
}

bool QirToQasmTranslator::translateToFile(llvm::Module* module,
                                          const std::string& outPath,
                                          const std::string& kernelName) {
  std::string qasm = translate(module, kernelName);
  if (qasm.empty()) return false;

  std::ofstream ofs(outPath);
  if (!ofs) return false;
  ofs << qasm;
  ofs.close();
  return true;
}

std::string qirToQasm(llvm::Module* module, const std::string& kernelName) {
  QirToQasmTranslator translator;
  return translator.translate(module, kernelName);
}

bool qirToQasmFile(llvm::Module* module, const std::string& outPath,
                   const std::string& kernelName) {
  QirToQasmTranslator translator;
  return translator.translateToFile(module, outPath, kernelName);
}

}  // namespace qllvm
