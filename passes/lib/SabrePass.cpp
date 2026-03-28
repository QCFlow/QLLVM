/*******************************************************************************
 * SabrePass - LLVM New PM integration + IR modification (Strategy B)
 *******************************************************************************/

#include "qllvm/passes/SabrePass.hpp"
#include "qllvm/passes/ApplyLayoutQirPass.hpp"
#include "qllvm/passes/CircuitExtractor.hpp"
#include "qllvm/passes/CouplingMap.hpp"
#include "qllvm/passes/SabreLayout.hpp"
#include "qllvm/passes/SabreSwap.hpp"

#include <llvm/IR/Constants.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/ErrorHandling.h>

#include <algorithm>
#include <vector>

namespace qllvm {
namespace sabre {

namespace {

/// Collect __quantum__rt__qubit_allocate_array calls in block order (matches
/// CircuitExtractor flat indexing).
static bool collectQubitAllocCalls(
    llvm::Function* kernel,
    std::vector<llvm::CallInst*>& out) {
  out.clear();
  if (!kernel) return false;
  for (auto& BB : *kernel) {
    for (auto& I : BB) {
      auto* CI = llvm::dyn_cast<llvm::CallInst>(&I);
      if (!CI || !CI->getCalledFunction()) continue;
      if (CI->getCalledFunction()->getName() ==
          "__quantum__rt__qubit_allocate_array")
        out.push_back(CI);
    }
  }
  return !out.empty();
}

static bool qubitAllocStats(llvm::Function* kernel, int& maxChunk,
                            int& sumChunks) {
  maxChunk = 0;
  sumChunks = 0;
  std::vector<llvm::CallInst*> allocs;
  if (!collectQubitAllocCalls(kernel, allocs)) return false;
  for (llvm::CallInst* CI : allocs) {
    auto* C = llvm::dyn_cast<llvm::ConstantInt>(CI->getArgOperand(0));
    if (!C) return false;
    int n = (int)C->getZExtValue();
    if (n < 1) return false;
    maxChunk = std::max(maxChunk, n);
    sumChunks += n;
  }
  return true;
}

/// Pad allocation(s) so total flat qubit count == numPhys (device width).
/// Multiple QASM qregs become multiple arrays; only the first grows so the
/// last register's flat index stays <= numPhys - 1.
bool expandQubitAllocToCouplingSize(llvm::Function* kernel, int numPhys) {
  if (!kernel || numPhys <= 0) return false;
  auto* i64Ty = llvm::Type::getInt64Ty(kernel->getContext());
  std::vector<llvm::CallInst*> allocs;
  if (!collectQubitAllocCalls(kernel, allocs)) return false;

  if (allocs.size() == 1) {
    auto* C = llvm::dyn_cast<llvm::ConstantInt>(allocs[0]->getArgOperand(0));
    if (!C) return false;
    int n = (int)C->getZExtValue();
    if (n < numPhys)
      allocs[0]->setArgOperand(0, llvm::ConstantInt::get(i64Ty, (uint64_t)numPhys));
    return true;
  }

  int rest = 0;
  for (size_t i = 1; i < allocs.size(); ++i) {
    auto* C = llvm::dyn_cast<llvm::ConstantInt>(allocs[i]->getArgOperand(0));
    if (!C) return false;
    rest += (int)C->getZExtValue();
  }
  if (rest >= numPhys) return false;
  int firstTarget = numPhys - rest;
  if (firstTarget < 1) return false;
  auto* C0 = llvm::dyn_cast<llvm::ConstantInt>(allocs[0]->getArgOperand(0));
  if (!C0) return false;
  int n0 = (int)C0->getZExtValue();
  if (n0 < firstTarget)
    allocs[0]->setArgOperand(
        0, llvm::ConstantInt::get(i64Ty, (uint64_t)firstTarget));
  return true;
}

llvm::Function* findKernel(llvm::Module& M, const std::string& kernelName) {
  for (auto& F : M) {
    if (F.isDeclaration()) continue;
    auto name = F.getName();
    if (name.startswith("__internal_mlir_")) {
      if (kernelName.empty()) return &F;
      if (name == "__internal_mlir_" + kernelName) return &F;
    }
  }
  return nullptr;
}

llvm::Function* getSwapFunc(llvm::Module& M) {
  llvm::Function* F = M.getFunction("__quantum__qis__swap");
  if (F) return F;
  auto* T = llvm::Type::getVoidTy(M.getContext());
  auto* PtrT = llvm::Type::getInt8PtrTy(M.getContext());
  llvm::FunctionType* FT = llvm::FunctionType::get(T, {PtrT, PtrT}, false);
  F = llvm::Function::Create(FT, llvm::Function::ExternalLinkage,
                             "__quantum__qis__swap", &M);
  return F;
}

}  // namespace

llvm::PreservedAnalyses SabrePass::run(llvm::Module& M,
                                       llvm::ModuleAnalysisManager& AM) {
  if (!opts_.enabled()) return llvm::PreservedAnalyses::all();

  llvm::Function* kernel = findKernel(M, opts_.kernelName);
  if (!kernel) return llvm::PreservedAnalyses::all();

  CouplingMap coupling(opts_.couplingEdges);
  const int numPhys = coupling.numQubits();
  if (numPhys <= 0) return llvm::PreservedAnalyses::all();

  int maxChunk = 0, sumChunks = 0;
  if (!qubitAllocStats(kernel, maxChunk, sumChunks))
    return llvm::PreservedAnalyses::all();
  if (maxChunk > numPhys || sumChunks > numPhys)
    return llvm::PreservedAnalyses::all();

  if (!expandQubitAllocToCouplingSize(kernel, numPhys))
    return llvm::PreservedAnalyses::all();

  CircuitExtractor extractor;
  Circuit circ;
  if (!extractor.extract(kernel, circ)) return llvm::PreservedAnalyses::all();

  if (circ.numQubits > coupling.numQubits())
    return llvm::PreservedAnalyses::all();

  SabreLayout layout(coupling, opts_);
  std::vector<int> initialLayout = layout.run(circ);

  SabreSwap swap(coupling, opts_);
  auto result = swap.run(circ, initialLayout);

  bool routingComplete = true;
  for (size_t gi = 0; gi < circ.gates.size(); ++gi) {
    if (result.layoutBeforeGate[gi].empty()) {
      routingComplete = false;
      break;
    }
  }
  if (!routingComplete) {
    result.insertions.clear();
    llvm::report_fatal_error(
        "SABRE routing did not complete (iteration/insertion limits or stale "
        "layout). Refuse to emit partial mapping; raise limits or simplify "
        "circuit.");
  }

  llvm::Function* gepFn = M.getFunction("__quantum__rt__array_get_element_ptr_1d");
  if (!gepFn && circ.qubitArrayAlloc) {
    auto* i64Ty = llvm::Type::getInt64Ty(M.getContext());
    auto* i8PtrTy = llvm::Type::getInt8PtrTy(M.getContext());
    auto* ft = llvm::FunctionType::get(
        i8PtrTy, {circ.qubitArrayAlloc->getType(), i64Ty}, false);
    gepFn = llvm::Function::Create(ft, llvm::Function::ExternalLinkage,
                                   "__quantum__rt__array_get_element_ptr_1d",
                                   &M);
  }

  bool modified = false;

  if (!result.insertions.empty()) {
    llvm::Function* swapFunc = getSwapFunc(M);
    modified = true;

    // Multiple swaps inserted with IRBuilder(InsertBefore gate) stack newer
    // instructions closer to `gate`, so naive forward insertion runs them in
    // reverse of SABRE simulation order. Group by anchor gate and emit each
    // group from last simulated swap to first, chaining InsertBefore so runtime
    // order matches simulation (and applyPhysicalLayoutQir's layout walk).
    const size_t nIns = result.insertions.size();
    size_t i = 0;
    while (i < nIns) {
      SwapInsertion& first = result.insertions[i];
      if (first.beforeGateIndex < 0 ||
          first.beforeGateIndex >= (int)circ.gates.size()) {
        ++i;
        continue;
      }
      llvm::CallInst* gateAnchor = circ.gates[first.beforeGateIndex].callInst;
      if (!gateAnchor) {
        ++i;
        continue;
      }

      size_t j = i + 1;
      while (j < nIns) {
        const SwapInsertion& sj = result.insertions[j];
        if (sj.beforeGateIndex < 0 ||
            sj.beforeGateIndex >= (int)circ.gates.size())
          break;
        llvm::CallInst* a = circ.gates[sj.beforeGateIndex].callInst;
        if (a != gateAnchor) break;
        ++j;
      }

      llvm::Instruction* insertPt = gateAnchor;
      for (size_t k = j; k > i;) {
        --k;
        SwapInsertion& ins = result.insertions[k];
        llvm::Value* val0 = ins.val0;
        llvm::Value* val1 = ins.val1;

        if ((!val0 || !val1) && gepFn && circ.qubitArrayAlloc) {
          llvm::IRBuilder<> builder(insertPt);
          auto* i64Ty = llvm::Type::getInt64Ty(M.getContext());
          if (!val0) {
            auto* idx = llvm::ConstantInt::get(i64Ty, ins.v0);
            val0 = builder.CreateCall(gepFn, {circ.qubitArrayAlloc, idx});
          }
          if (!val1) {
            auto* idx = llvm::ConstantInt::get(i64Ty, ins.v1);
            val1 = builder.CreateCall(gepFn, {circ.qubitArrayAlloc, idx});
          }
        }

        if (!val0 || !val1) {
          llvm::report_fatal_error(
              "SabrePass: missing qubit SSA for SWAP; cannot preserve "
              "equivalence.");
        }

        llvm::IRBuilder<> builder(insertPt);
        llvm::Value* args[] = {val0, val1};
        ins.swapCall = builder.CreateCall(swapFunc, args);
        insertPt = ins.swapCall;
      }

      i = j;
    }
  }

  if (opts_.applyPhysicalLayoutToQir && circ.qubitArrayAlloc && gepFn) {
    if (applyPhysicalLayoutQir(kernel, circ.qubitArrayAlloc, gepFn,
                               coupling.numQubits(), initialLayout, circ,
                               result))
      modified = true;
  }

  return modified ? llvm::PreservedAnalyses::none()
                  : llvm::PreservedAnalyses::all();
}

bool runSabre(llvm::Module* module, const SabreOptions& opts,
              const std::string& kernelName) {
  if (!module || !opts.enabled()) return true;
  SabreOptions o = opts;
  o.kernelName = kernelName;
  SabrePass pass(std::move(o));
  llvm::ModuleAnalysisManager AM;
  pass.run(*module, AM);
  return true;
}

}  // namespace sabre
}  // namespace qllvm
