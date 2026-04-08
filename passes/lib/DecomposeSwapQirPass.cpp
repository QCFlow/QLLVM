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
 * DecomposeSwapQirPass - decompose __quantum__qis__swap in QIR by basicGateSet
 *******************************************************************************/

#include "qllvm/passes/DecomposeSwapQirPass.hpp"

#include <cmath>
#include <llvm/IR/Constants.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <iostream>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#ifndef M_PI_2
#define M_PI_2 (M_PI / 2.0)
#endif

namespace qllvm {
namespace sabre {

namespace {

std::unordered_set<std::string> basic_Gate_Set;

bool hasGate(const std::unordered_set<std::string>& gateSet,
             const std::string& gate) {
  return gateSet.count(gate) != 0;
}

llvm::Function* getOrCreateCnot(llvm::Module* M) {
  auto* F = M->getFunction("__quantum__qis__cnot");
  if (F) return F;
  auto* T = llvm::Type::getVoidTy(M->getContext());
  auto* PtrT = llvm::Type::getInt8PtrTy(M->getContext());
  auto* FT = llvm::FunctionType::get(T, {PtrT, PtrT}, false);
  return llvm::Function::Create(FT, llvm::Function::ExternalLinkage,
                                "__quantum__qis__cnot", M);
}

llvm::Function* getOrCreateH(llvm::Module* M) {
  auto* F = M->getFunction("__quantum__qis__h");
  if (F) return F;
  auto* T = llvm::Type::getVoidTy(M->getContext());
  auto* PtrT = llvm::Type::getInt8PtrTy(M->getContext());
  auto* FT = llvm::FunctionType::get(T, {PtrT}, false);
  return llvm::Function::Create(FT, llvm::Function::ExternalLinkage,
                                "__quantum__qis__h", M);
}

llvm::Function* getOrCreateSx(llvm::Module* M) {
  auto* F = M->getFunction("__quantum__qis__sx");
  if (F) return F;
  auto* T = llvm::Type::getVoidTy(M->getContext());
  auto* PtrT = llvm::Type::getInt8PtrTy(M->getContext());
  auto* FT = llvm::FunctionType::get(T, {PtrT}, false);
  return llvm::Function::Create(FT, llvm::Function::ExternalLinkage,
                                "__quantum__qis__sx", M);
}
llvm::Function* getOrCreateX(llvm::Module* M) {
  auto* F = M->getFunction("__quantum__qis__x");
  if (F) return F;
  auto* T = llvm::Type::getVoidTy(M->getContext());
  auto* PtrT = llvm::Type::getInt8PtrTy(M->getContext());
  auto* FT = llvm::FunctionType::get(T, {PtrT}, false);
  return llvm::Function::Create(FT, llvm::Function::ExternalLinkage,
                                "__quantum__qis__x", M);
}

llvm::Function* getOrCreateCz(llvm::Module* M) {
  auto* F = M->getFunction("__quantum__qis__cz");
  if (F) return F;
  auto* T = llvm::Type::getVoidTy(M->getContext());
  auto* PtrT = llvm::Type::getInt8PtrTy(M->getContext());
  auto* FT = llvm::FunctionType::get(T, {PtrT, PtrT}, false);
  return llvm::Function::Create(FT, llvm::Function::ExternalLinkage,
                                "__quantum__qis__cz", M);
}

llvm::Function* getOrCreateRz(llvm::Module* M) {
  auto* F = M->getFunction("__quantum__qis__rz");
  if (F) return F;
  auto* T = llvm::Type::getVoidTy(M->getContext());
  auto* DblT = llvm::Type::getDoubleTy(M->getContext());
  auto* PtrT = llvm::Type::getInt8PtrTy(M->getContext());
  auto* FT = llvm::FunctionType::get(T, {DblT, PtrT}, false);
  return llvm::Function::Create(FT, llvm::Function::ExternalLinkage,
                                "__quantum__qis__rz", M);
}

llvm::Function* getOrCreateRy(llvm::Module* M) {
  auto* F = M->getFunction("__quantum__qis__ry");
  if (F) return F;
  auto* T = llvm::Type::getVoidTy(M->getContext());
  auto* DblT = llvm::Type::getDoubleTy(M->getContext());
  auto* PtrT = llvm::Type::getInt8PtrTy(M->getContext());
  auto* FT = llvm::FunctionType::get(T, {DblT, PtrT}, false);
  return llvm::Function::Create(FT, llvm::Function::ExternalLinkage,
                                "__quantum__qis__ry", M);
}

llvm::Function* getOrCreateRx(llvm::Module* M) {
  auto* F = M->getFunction("__quantum__qis__rx");
  if (F) return F;
  auto* T = llvm::Type::getVoidTy(M->getContext());
  auto* DblT = llvm::Type::getDoubleTy(M->getContext());
  auto* PtrT = llvm::Type::getInt8PtrTy(M->getContext());
  auto* FT = llvm::FunctionType::get(T, {DblT, PtrT}, false);
  return llvm::Function::Create(FT, llvm::Function::ExternalLinkage,
                                "__quantum__qis__rx", M);
}

llvm::Function* getOrCreateU3(llvm::Module* M) {
  auto* F = M->getFunction("__quantum__qis__u3");
  if (F) return F;
  auto* T = llvm::Type::getVoidTy(M->getContext());
  auto* DblT = llvm::Type::getDoubleTy(M->getContext());
  auto* PtrT = llvm::Type::getInt8PtrTy(M->getContext());
  auto* FT = llvm::FunctionType::get(T, {DblT, DblT, DblT, PtrT}, false);
  return llvm::Function::Create(FT, llvm::Function::ExternalLinkage,
                                "__quantum__qis__u3", M);
}

// according to the basic gate set, select the appropriate gate to decompose the H gate
void emitHOrDecompose(llvm::Module* M, llvm::IRBuilder<>& builder,
                     llvm::Value* qubit) {
  auto& Ctx = M->getContext();
  auto* DblT = llvm::Type::getDoubleTy(Ctx);
  auto pi = llvm::ConstantFP::get(DblT, M_PI);
  auto pi2 = llvm::ConstantFP::get(DblT, M_PI_2);
  auto zero = llvm::ConstantFP::get(DblT, 0.0);

  if (hasGate(basic_Gate_Set, "h")) {
    auto* hFn = getOrCreateH(M);
    builder.CreateCall(hFn, {qubit});
    return;
  }
  if (hasGate(basic_Gate_Set, "ry") && hasGate(basic_Gate_Set, "rz")) {
    auto* rzFn = getOrCreateRz(M);
    auto* ryFn = getOrCreateRy(M);
    builder.CreateCall(rzFn, {pi, qubit});
    builder.CreateCall(ryFn, {pi2, qubit});
    return;
  }else if (hasGate(basic_Gate_Set, "rx") && hasGate(basic_Gate_Set, "rz")) {
    auto* rzFn = getOrCreateRz(M);
    auto* rxFn = getOrCreateRx(M);
    builder.CreateCall(rzFn, {pi2, qubit});
    builder.CreateCall(rxFn, {pi2, qubit});
    builder.CreateCall(rzFn, {pi2, qubit});
    return;
  }else if (hasGate(basic_Gate_Set, "u3")) {
    auto* u3Fn = getOrCreateU3(M);
    builder.CreateCall(u3Fn, {pi2, zero, pi, qubit});
    return;
  }else if (hasGate(basic_Gate_Set, "sx") && hasGate(basic_Gate_Set, "rz") && hasGate(basic_Gate_Set, "x")) {
    auto* rzFn = getOrCreateRz(M);
    auto* sxFn = getOrCreateSx(M);
    auto* xFn = getOrCreateX(M);
    builder.CreateCall(rzFn, {pi, qubit});
    builder.CreateCall(sxFn, {qubit});
    builder.CreateCall(rzFn, {pi2, qubit});
    builder.CreateCall(sxFn, {qubit});
    builder.CreateCall(xFn, {qubit});

    return;
  }else{
      assert(false && "Not support to trans H to the given basic gate");
  }
}

// use the decomposition method with CZ
void decomposeToCzH(llvm::Module* M, llvm::CallInst* CI) {
  llvm::Value* a = CI->getArgOperand(0);
  llvm::Value* b = CI->getArgOperand(1);
  auto* czFn = getOrCreateCz(M);
  llvm::IRBuilder<> builder(CI);
  // CNOT(a,b) = H(b); CZ(a,b); H(b)
  emitHOrDecompose(M, builder, b);
  builder.CreateCall(czFn, {a, b});
  emitHOrDecompose(M, builder, b);
  // CNOT(b,a) = H(a); CZ(b,a); H(a)
  emitHOrDecompose(M, builder, a);
  builder.CreateCall(czFn, {b, a});
  emitHOrDecompose(M, builder, a);
  // CNOT(a,b)
  emitHOrDecompose(M, builder, b);
  builder.CreateCall(czFn, {a, b});
  emitHOrDecompose(M, builder, b);
  CI->eraseFromParent();
}

// use the decomposition method with CX
void decomposeTocx(llvm::Module* M, llvm::CallInst* CI) {
  llvm::Value* a = CI->getArgOperand(0);
  llvm::Value* b = CI->getArgOperand(1);
  auto* cnotFn = getOrCreateCnot(M);
  llvm::IRBuilder<> builder(CI);
  builder.CreateCall(cnotFn, {a, b});
  builder.CreateCall(cnotFn, {b, a});
  builder.CreateCall(cnotFn, {a, b});
  CI->eraseFromParent();
}


// according to the basic gate set, select the appropriate gate to decompose the SWAP gate
void decomposeswap(llvm::Module* M, llvm::CallInst* CI) {
  
  if(hasGate(basic_Gate_Set, "cx")) {
    decomposeTocx(M, CI);
  }else if(hasGate(basic_Gate_Set, "cz")){
    decomposeToCzH(M, CI);
  }else{
    assert(false && "Not support to trans SWAP to the given basic gate");
  }
}

}  // namespace

bool runDecomposeSwapQir(
    llvm::Module* module,
    const std::unordered_set<std::string>& basicGateSet) {
  if (!module) return true;

  bool keepSwap = false;
  
  basic_Gate_Set = basicGateSet;
  if(basic_Gate_Set.size() == 0){
    basic_Gate_Set = {"swap"};
  }

  if (hasGate(basic_Gate_Set, "swap")) {
    keepSwap = true;
  }
  
  if (keepSwap) return true;

  std::vector<llvm::CallInst*> toProcess;
  for (auto& F : *module) {
    for (auto& BB : F) {
      for (auto& I : BB) {
        auto* CI = llvm::dyn_cast<llvm::CallInst>(&I);
        if (!CI || !CI->getCalledFunction()) continue;
        if (CI->getCalledFunction()->getName() != "__quantum__qis__swap")
          continue;
        toProcess.push_back(CI);
      }
    }
  }


  for (auto* CI : toProcess) {
    decomposeswap(module, CI);
  }
  return true;
}

}  // namespace sabre
}  // namespace qllvm
