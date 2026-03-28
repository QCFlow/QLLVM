/*******************************************************************************
 * ApplyLayoutQirPass - map logical qubit SSA to physical GEP indices in QIR
 *******************************************************************************/

#include "qllvm/passes/ApplyLayoutQirPass.hpp"

#include <llvm/IR/Constants.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>

#include <unordered_map>
#include <vector>

namespace qllvm {
namespace sabre {

namespace {

llvm::CallInst* traceToGepCall(llvm::Value* V) {
  if (!V) return nullptr;
  if (auto* LI = llvm::dyn_cast<llvm::LoadInst>(V)) {
    llvm::Value* ptr = LI->getPointerOperand();
    if (auto* BC = llvm::dyn_cast<llvm::BitCastInst>(ptr))
      ptr = BC->getOperand(0);
    V = ptr;
  }
  if (auto* BC = llvm::dyn_cast<llvm::BitCastInst>(V))
    V = BC->getOperand(0);
  auto* GEP = llvm::dyn_cast<llvm::CallInst>(V);
  if (!GEP || !GEP->getCalledFunction()) return nullptr;
  if (GEP->getCalledFunction()->getName() !=
      "__quantum__rt__array_get_element_ptr_1d")
    return nullptr;
  return GEP;
}

/// Flat logical index (same convention as CircuitExtractor::buildQubitMap).
int flatLogicalFromQubitArg(llvm::Value* arg, llvm::Function* kernel) {
  if (!arg || !kernel) return -1;
  auto* GEP = traceToGepCall(arg);
  if (!GEP) return -1;
  llvm::Value* arr = GEP->getArgOperand(0);
  auto* CIdx = llvm::dyn_cast<llvm::ConstantInt>(GEP->getArgOperand(1));
  if (!CIdx) return -1;
  int idx = (int)CIdx->getZExtValue();

  int base = 0;
  for (auto& BB : *kernel) {
    for (auto& I : BB) {
      auto* CI = llvm::dyn_cast<llvm::CallInst>(&I);
      if (!CI || !CI->getCalledFunction()) continue;
      if (CI->getCalledFunction()->getName() !=
          "__quantum__rt__qubit_allocate_array")
        continue;
      auto* CN = llvm::dyn_cast<llvm::ConstantInt>(CI->getArgOperand(0));
      if (!CN) continue;
      int N = (int)CN->getZExtValue();
      if (CI == arr) return base + idx;
      base += N;
    }
  }
  return -1;
}

llvm::Value* buildQubitLoad(llvm::IRBuilderBase& IR, llvm::Value* alloc,
                            llvm::Function* gepFn, int physicalIdx,
                            llvm::LoadInst* origLoad) {
  llvm::LLVMContext& Ctx = IR.getContext();
  auto* i64Ty = llvm::Type::getInt64Ty(Ctx);
  auto* idx = llvm::ConstantInt::get(i64Ty, (uint64_t)physicalIdx);
  llvm::CallInst* gepCall = IR.CreateCall(gepFn, {alloc, idx});
  llvm::Value* ptr = gepCall;
  llvm::Value* origPtr = origLoad->getPointerOperand();
  if (auto* BC = llvm::dyn_cast<llvm::BitCastInst>(origPtr))
    ptr = IR.CreateBitCast(gepCall, BC->getType());
  llvm::LoadInst* L = IR.CreateLoad(origLoad->getType(), ptr);
  L->setAlignment(origLoad->getAlign());
  return L;
}

void rewriteGepQubitArg(llvm::IRBuilderBase& IR, llvm::CallInst* gateCall,
                        unsigned ai, llvm::Value* alloc, llvm::Function* gepFn,
                        int physicalIdx, llvm::Value* origArg,
                        llvm::Type* i64Ty) {
  auto* idx = llvm::ConstantInt::get(i64Ty, (uint64_t)physicalIdx);
  llvm::CallInst* newGep = IR.CreateCall(gepFn, {alloc, idx});
  llvm::Value* repl = newGep;
  if (llvm::isa<llvm::BitCastInst>(origArg))
    repl = IR.CreateBitCast(newGep, origArg->getType());
  gateCall->setArgOperand(ai, repl);
}

std::vector<unsigned> qubitArgIndices(llvm::StringRef name) {
  if (name == "__quantum__qis__rx" || name == "__quantum__qis__ry" ||
      name == "__quantum__qis__rz" || name == "__quantum__qis__p")
    return {1};
  if (name == "__quantum__qis__u3" || name == "__quantum__qis__su2")
    return {3};
  if (name == "__quantum__qis__cnot" || name == "__quantum__qis__cz" ||
      name == "__quantum__qis__swap")
    return {0, 1};
  if (name == "__quantum__qis__cp" || name == "__quantum__qis__cphase")
    return {1, 2};
  if (name == "__quantum__qis__mz" || name == "__quantum__qis__reset")
    return {0};
  if (name == "__quantum__qis__h" || name == "__quantum__qis__x" ||
      name == "__quantum__qis__y" || name == "__quantum__qis__z" ||
      name == "__quantum__qis__s" || name == "__quantum__qis__sx" ||
      name == "__quantum__qis__sdg" || name == "__quantum__qis__t" ||
      name == "__quantum__qis__tdg" || name == "__quantum__qis__X2P" ||
      name == "__quantum__qis__X2M" || name == "__quantum__qis__Y2P")
    return {0};
  return {};
}

/// Keep total flat qubit array size == numPhys. Same split as expandQubitAllocToCouplingSize.
static void syncQubitAllocSizesToDevice(llvm::Function* kernel, int numPhys) {
  if (!kernel || numPhys <= 0) return;
  std::vector<llvm::CallInst*> allocs;
  for (auto& BB : *kernel) {
    for (auto& I : BB) {
      auto* CI = llvm::dyn_cast<llvm::CallInst>(&I);
      if (!CI || !CI->getCalledFunction()) continue;
      if (CI->getCalledFunction()->getName() ==
          "__quantum__rt__qubit_allocate_array")
        allocs.push_back(CI);
    }
  }
  if (allocs.empty()) return;
  llvm::LLVMContext& Ctx = kernel->getContext();
  auto* i64Ty = llvm::Type::getInt64Ty(Ctx);
  if (allocs.size() == 1) {
    allocs[0]->setArgOperand(
        0, llvm::ConstantInt::get(i64Ty, (uint64_t)numPhys));
    return;
  }
  int rest = 0;
  for (size_t i = 1; i < allocs.size(); ++i) {
    auto* C = llvm::dyn_cast<llvm::ConstantInt>(allocs[i]->getArgOperand(0));
    if (!C) return;
    rest += (int)C->getZExtValue();
  }
  if (rest >= numPhys) return;
  int firstSize = numPhys - rest;
  if (firstSize < 1) return;
  allocs[0]->setArgOperand(
      0, llvm::ConstantInt::get(i64Ty, (uint64_t)firstSize));
}

void rewriteQisCallOperands(llvm::Function* kernel, llvm::CallInst* CI,
                            llvm::Value* qubitAlloc, llvm::Function* gepFn,
                            int numPhys, const std::vector<int>& physMap) {
  if (!kernel || !CI || !CI->getCalledFunction()) return;
  llvm::StringRef name = CI->getCalledFunction()->getName();
  if (!name.startswith("__quantum__qis__")) return;

  std::vector<unsigned> qIdx = qubitArgIndices(name);
  if (qIdx.empty()) return;

  int numLogical = (int)physMap.size();
  llvm::LLVMContext& Ctx = CI->getContext();
  auto* i64Ty = llvm::Type::getInt64Ty(Ctx);

  llvm::IRBuilder<> IR(CI);
  for (unsigned ai : qIdx) {
    if (ai >= CI->getNumArgOperands()) continue;
    llvm::Value* arg = CI->getArgOperand(ai);
    int logical = flatLogicalFromQubitArg(arg, kernel);
    if (logical < 0 || logical >= numLogical) continue;
    int phys = physMap[logical];
    if (phys < 0 || phys >= numPhys) continue;
    if (auto* origLoad = llvm::dyn_cast<llvm::LoadInst>(arg)) {
      llvm::Value* newLoad =
          buildQubitLoad(IR, qubitAlloc, gepFn, phys, origLoad);
      CI->setArgOperand(ai, newLoad);
    } else {
      rewriteGepQubitArg(IR, CI, ai, qubitAlloc, gepFn, phys, arg, i64Ty);
    }
  }
}

}  // namespace

bool applyPhysicalLayoutQir(llvm::Function* kernel, llvm::Value* qubitAlloc,
                            llvm::Function* gepFn, int numPhys,
                            const std::vector<int>& initialLayout,
                            const Circuit& circ,
                            const SabreSwapResult& sabreResult) {
  if (!kernel || !qubitAlloc || !gepFn || numPhys <= 0) return false;
  if (initialLayout.empty()) return false;

  auto* allocCI = llvm::dyn_cast<llvm::CallInst>(qubitAlloc);
  if (!allocCI || !allocCI->getCalledFunction() ||
      allocCI->getCalledFunction()->getName() !=
          "__quantum__rt__qubit_allocate_array")
    return false;

  syncQubitAllocSizesToDevice(kernel, numPhys);

  std::unordered_map<llvm::CallInst*, int> gateIndexByCall;
  gateIndexByCall.reserve(circ.gates.size());
  for (int gi = 0; gi < (int)circ.gates.size(); ++gi) {
    if (circ.gates[gi].callInst)
      gateIndexByCall[circ.gates[gi].callInst] = gi;
  }

  std::unordered_map<llvm::CallInst*, const SwapInsertion*> sabreSwapByCall;
  sabreSwapByCall.reserve(sabreResult.insertions.size());
  for (const auto& ins : sabreResult.insertions) {
    if (ins.swapCall) sabreSwapByCall[ins.swapCall] = &ins;
  }

  std::vector<int> layout = initialLayout;
  const int numLogical = (int)layout.size();

  for (llvm::BasicBlock& BB : *kernel) {
    for (llvm::Instruction& I : BB) {
      auto* CI = llvm::dyn_cast<llvm::CallInst>(&I);
      if (!CI || !CI->getCalledFunction()) continue;
      llvm::StringRef name = CI->getCalledFunction()->getName();

      if (name == "__quantum__qis__swap") {
        auto it = sabreSwapByCall.find(CI);
        if (it != sabreSwapByCall.end()) {
          const SwapInsertion* ins = it->second;
          // Use the snapshot from SABRE simulation so layout cannot drift from
          // incremental swap updates (e.g. swap grouping / future IR changes).
          if (!ins->layoutBeforeSwap.empty() &&
              (int)ins->layoutBeforeSwap.size() == numLogical)
            layout = ins->layoutBeforeSwap;
          rewriteQisCallOperands(kernel, CI, qubitAlloc, gepFn, numPhys,
                                 layout);
          if (ins->v0 >= 0 && ins->v0 < numLogical && ins->v1 >= 0 &&
              ins->v1 < numLogical)
            std::swap(layout[ins->v0], layout[ins->v1]);
        } else {
          int v0 = flatLogicalFromQubitArg(CI->getArgOperand(0), kernel);
          int v1 = flatLogicalFromQubitArg(CI->getArgOperand(1), kernel);
          rewriteQisCallOperands(kernel, CI, qubitAlloc, gepFn, numPhys,
                                 layout);
          if (v0 >= 0 && v0 < numLogical && v1 >= 0 && v1 < numLogical)
            std::swap(layout[v0], layout[v1]);
        }
        continue;
      }

      if (!name.startswith("__quantum__qis__")) continue;
      auto git = gateIndexByCall.find(CI);
      if (git == gateIndexByCall.end()) continue;
      int gi = git->second;
      if (gi >= 0 && gi < (int)sabreResult.layoutBeforeGate.size() &&
          !sabreResult.layoutBeforeGate[gi].empty() &&
          (int)sabreResult.layoutBeforeGate[gi].size() == numLogical)
        layout = sabreResult.layoutBeforeGate[gi];
      rewriteQisCallOperands(kernel, CI, qubitAlloc, gepFn, numPhys, layout);
    }
  }

  return true;
}

}  // namespace sabre
}  // namespace qllvm
