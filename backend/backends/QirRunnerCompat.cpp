/*******************************************************************************
 * QirRunnerCompat implementation
 *******************************************************************************/

#include "qllvm/QirRunnerCompat.hpp"
#include "llvm/IR/Constants.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/Support/raw_ostream.h"

#include <string>

namespace qllvm {

/// Remove all calls to F and erase F; used for unsupported runtime functions
static void removeCallsAndFunction(llvm::Module* M, llvm::Function* F) {
  if (!F) return;
  std::vector<llvm::CallBase*> toRemove;
  for (llvm::Use& U : F->uses()) {
    if (auto* CB = llvm::dyn_cast<llvm::CallBase>(U.getUser()))
      toRemove.push_back(CB);
  }
  for (auto* CB : toRemove)
    CB->eraseFromParent();
  F->eraseFromParent();
}

/// Rename __quantum__qis__X to __quantum__qis__X__body (except mz, handled below)
static void renameQisToBody(llvm::Module* M) {
  std::vector<llvm::Function*> toRename;
  for (llvm::Function& F : *M) {
    llvm::StringRef name = F.getName();
    if (name == "__quantum__qis__mz") continue;  // mz has different ABI, handled in adaptMzCalls
    if (name.startswith("__quantum__qis__") && !name.endswith("__body") &&
        !name.endswith("__ctl") && !name.endswith("__ctladj") &&
        !name.endswith("__adj"))
      toRename.push_back(&F);
  }
  for (llvm::Function* F : toRename)
    F->setName(F->getName().str() + "__body");
}

/// Transform mz(qubit)->Result* to mz(qubit, result_id) for qir-runner.
/// Returns the number of measurement results.
static size_t adaptMzCalls(llvm::Module* M) {
  // renameQisToBody will change mz to mz__body, so first check mz__body
  llvm::Function* mzFn = M->getFunction("__quantum__qis__mz__body");
  if (!mzFn) mzFn = M->getFunction("__quantum__qis__mz");
  if (!mzFn || mzFn->arg_size() != 1) return 0;

  llvm::LLVMContext& ctx = M->getContext();
  llvm::IRBuilder<> builder(ctx);

  llvm::Type* resultPtr = mzFn->getReturnType();
  llvm::IntegerType* i64 = llvm::Type::getInt64Ty(ctx);

  llvm::FunctionType* newMzTy = llvm::FunctionType::get(
      llvm::Type::getVoidTy(ctx),
      {mzFn->getArg(0)->getType(), resultPtr}, false);
  std::string oldName = mzFn->getName().str();
  mzFn->setName(oldName + "_replaced");
  llvm::Function* newMz = llvm::Function::Create(
      newMzTy, llvm::Function::ExternalLinkage,
      "__quantum__qis__mz__body", M);

  std::vector<llvm::CallBase*> toReplace;
  for (llvm::Use& U : mzFn->uses()) {
    if (auto* CB = llvm::dyn_cast<llvm::CallBase>(U.getUser()))
      toReplace.push_back(CB);
  }
  size_t numResults = toReplace.size();
  for (size_t i = 0; i < toReplace.size(); ++i) {
    llvm::CallBase* CB = toReplace[i];
    llvm::Value* qubit = CB->getArgOperand(0);
    // inttoptr(i64 0) canonicalizes to null in LLVM; qir-runner needs non-null tags.
    llvm::Value* resId = llvm::ConstantExpr::getIntToPtr(
        llvm::ConstantInt::get(i64, static_cast<uint64_t>(i) + 1u), resultPtr);
    builder.SetInsertPoint(CB);
    builder.CreateCall(newMz, {qubit, resId});
    CB->replaceAllUsesWith(resId);
    CB->eraseFromParent();
  }
  mzFn->eraseFromParent();
  return numResults;
}

/// MLIR may emit __quantum__qis__mz__body(%Qubit*, %Result*) with a null Result*
/// for some measures; adaptMzCalls only handles the 1-arg mz form. Loading via
/// bitcast(null) crashes qir-runner. Renumber all mz calls to use inttoptr(1..n)
/// and rewire the bitcast that reads back each result (next matching bitcast in
/// the same basic block).
static llvm::Function* stripToFunction(llvm::Value* v) {
  if (!v) return nullptr;
  v = v->stripPointerCasts();
  return llvm::dyn_cast<llvm::Function>(v);
}

/// qir-runner records shot bits via __quantum__rt__result_record_output; the LLVM
/// lowering still emits host loads from bitcast(Result* inttoptr k) -> i1*, which
/// JIT would dereference as a real address (e.g. 0x1) and crash. Replace those
/// loads with false; the simulator output does not use these classical shadows.
static void neutralizeResultIdHostLoads(llvm::Function* F) {
  if (!F || F->isDeclaration()) return;
  llvm::LLVMContext& ctx = F->getContext();
  llvm::Constant* cFalse = llvm::ConstantInt::getFalse(ctx);
  std::vector<llvm::Instruction*> toErase;

  for (llvm::BasicBlock& BB : *F) {
    for (llvm::Instruction& I : BB) {
      auto* load = llvm::dyn_cast<llvm::LoadInst>(&I);
      if (!load || !load->getType()->isIntegerTy(1)) continue;
      auto* bc = llvm::dyn_cast<llvm::BitCastInst>(load->getPointerOperand());
      if (!bc) continue;
      llvm::Value* src = bc->getOperand(0);
      auto* ce = llvm::dyn_cast<llvm::ConstantExpr>(src);
      if (!ce || ce->getOpcode() != llvm::Instruction::IntToPtr) continue;

      load->replaceAllUsesWith(cFalse);
      toErase.push_back(load);
    }
  }
  for (llvm::Instruction* inst : toErase) {
    if (inst->use_empty()) inst->eraseFromParent();
  }
}

static void fixTwoArgMzBodyResultSlots(llvm::Module* M) {
  llvm::Function* mzFn = M->getFunction("__quantum__qis__mz__body");
  if (!mzFn || mzFn->arg_size() != 2) return;
  if (!mzFn->getReturnType()->isVoidTy()) return;

  llvm::Type* resPtrTy = mzFn->getArg(1)->getType();
  if (!resPtrTy->isPointerTy()) return;
  llvm::LLVMContext& ctx = M->getContext();
  llvm::IntegerType* i64 = llvm::Type::getInt64Ty(ctx);
  unsigned nextId = 1;

  for (llvm::Function& F : *M) {
    if (F.isDeclaration()) continue;
    for (llvm::BasicBlock& BB : F) {
      for (llvm::Instruction& I : BB) {
        auto* call = llvm::dyn_cast<llvm::CallInst>(&I);
        if (!call) continue;
        llvm::Function* callee = call->getCalledFunction();
        if (!callee) callee = stripToFunction(call->getCalledOperand());
        if (!callee || callee->getName() != "__quantum__qis__mz__body") continue;

        llvm::Value* newRes = llvm::ConstantExpr::getIntToPtr(
            llvm::ConstantInt::get(i64, nextId++), resPtrTy);
        llvm::Value* oldRes = call->getArgOperand(1);
        call->setArgOperand(1, newRes);

        for (llvm::Instruction* pos = call->getNextNode(); pos;
             pos = pos->getNextNode()) {
          auto* bc = llvm::dyn_cast<llvm::BitCastInst>(pos);
          if (!bc) continue;
          if (bc->getOperand(0) != oldRes) continue;
          bc->setOperand(0, newRes);
          break;
        }
      }
    }
  }
}

static size_t countMzBodyCalls(llvm::Function* kernelFn) {
  llvm::Function* mzFn =
      kernelFn->getParent()->getFunction("__quantum__qis__mz__body");
  if (!mzFn) return 0;
  size_t n = 0;
  for (llvm::BasicBlock& BB : *kernelFn)
    for (llvm::Instruction& I : BB) {
      auto* call = llvm::dyn_cast<llvm::CallInst>(&I);
      if (!call) continue;
      llvm::Function* callee = call->getCalledFunction();
      if (!callee) callee = stripToFunction(call->getCalledOperand());
      if (callee && callee->getName() == "__quantum__qis__mz__body") ++n;
    }
  return n;
}

static std::string findKernelName(llvm::Module* M, const std::string& hint) {
  for (auto& F : *M) {
    if (F.isDeclaration()) continue;
    llvm::StringRef name = F.getName();
    if (name.startswith("__internal_mlir_")) {
      if (hint.empty()) return name.str();
      if (name == "__internal_mlir_" + hint) return name.str();
    }
  }
  for (auto& F : *M) {
    if (F.isDeclaration()) continue;
    if (F.getName().startswith("__internal_mlir_"))
      return F.getName().str();
  }
  return "";
}

bool adaptModuleForQirRunner(llvm::Module* module,
                            const std::string& kernelName) {
  if (!module) return false;

  llvm::LLVMContext& ctx = module->getContext();
  llvm::IRBuilder<> builder(ctx);

  // 0. Remove unsupported runtime calls (qir-runner has no implementations)
  removeCallsAndFunction(module, module->getFunction("__quantum__rt__set_config_parameter"));
  removeCallsAndFunction(module, module->getFunction("__quantum__rt__finalize"));
  removeCallsAndFunction(module, module->getFunction("__quantum__rt__set_external_qreg"));

  // 0b. Rename __quantum__qis__* to __quantum__qis__*__body
  renameQisToBody(module);

  std::string kernel = findKernelName(module, kernelName);
  if (kernel.empty()) return false;

  llvm::Function* kernelFn = module->getFunction(kernel);
  if (!kernelFn) return false;

  // remove main, make EntryPoint the only entry, mz only comes from kernel
  llvm::Function* mainFn = module->getFunction("main");
  if (mainFn) mainFn->eraseFromParent();

  // 0c. Adapt mz(qubit)->Result* to mz(qubit,result_ptr) ABI
  (void)adaptMzCalls(module);
  /// 1-arg mz may be rewritten above, but MLIR often also emits 2-arg mz__body with a
  /// null Result*; adaptMz then returns early and would skip fixing those calls.
  fixTwoArgMzBodyResultSlots(module);
  neutralizeResultIdHostLoads(kernelFn);
  size_t numResults = countMzBodyCalls(kernelFn);

  llvm::Function* initFn = module->getFunction("__quantum__rt__initialize");
  if (!initFn) return false;

  // 1. Change __quantum__rt__initialize to void(i8*)
  llvm::PointerType* i8ptr = llvm::Type::getInt8PtrTy(ctx);
  llvm::FunctionType* newInitTy =
      llvm::FunctionType::get(llvm::Type::getVoidTy(ctx), {i8ptr}, false);

  initFn->setName("__quantum__rt__initialize_old");
  llvm::Function* newInit = llvm::Function::Create(
      newInitTy, llvm::Function::ExternalLinkage,
      "__quantum__rt__initialize", module);

  // Replace all calls to old init with new call void __quantum__rt__initialize(i8* null)
  llvm::Value* nullPtr = llvm::ConstantPointerNull::get(i8ptr);
  std::vector<llvm::CallBase*> toReplace;
  for (llvm::Use& U : initFn->uses()) {
    if (auto* CB = llvm::dyn_cast<llvm::CallBase>(U.getUser()))
      toReplace.push_back(CB);
  }
  for (llvm::CallBase* CB : toReplace) {
    builder.SetInsertPoint(CB);
    builder.CreateCall(newInit, {nullPtr});
    CB->eraseFromParent();
  }
  initFn->eraseFromParent();

  // 2. Create EntryPoint() with "EntryPoint" attribute

  llvm::FunctionType* epTy =
      llvm::FunctionType::get(llvm::Type::getVoidTy(ctx), {}, false);
  llvm::Function* entryPoint = llvm::Function::Create(
      epTy, llvm::Function::ExternalLinkage, "EntryPoint", module);
  entryPoint->addFnAttr("EntryPoint");

  llvm::BasicBlock* bb =
      llvm::BasicBlock::Create(ctx, "entry", entryPoint);
  builder.SetInsertPoint(bb);
  builder.CreateCall(newInit, {nullPtr});
  builder.CreateCall(kernelFn);

  // 3. Inject the "result_record_output" call to output the measurement results for qir-runner to display.
  llvm::Function* recordOutputFn = module->getFunction("__quantum__rt__result_record_output");
  if (!recordOutputFn) {
    llvm::FunctionType* recordTy =
        llvm::FunctionType::get(llvm::Type::getVoidTy(ctx), {i8ptr}, false);
    recordOutputFn = llvm::Function::Create(
        recordTy, llvm::Function::ExternalLinkage,
        "__quantum__rt__result_record_output", module);
  }
  llvm::IntegerType* i64 = llvm::Type::getInt64Ty(ctx);
  for (size_t i = 0; i < numResults; ++i) {
    llvm::Value* resId = llvm::ConstantExpr::getIntToPtr(
        llvm::ConstantInt::get(i64, static_cast<uint64_t>(i) + 1u), i8ptr);
    builder.CreateCall(recordOutputFn, {resId});
  }

  builder.CreateRetVoid();

  return true;
}

}  // namespace qllvm
