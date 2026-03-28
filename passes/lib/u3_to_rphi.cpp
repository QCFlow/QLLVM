/*******************************************************************************
 * u3_to_rphi - decompose __quantum__qis__u3 in QIR by basicGateSet
 *******************************************************************************/

#include "qllvm/passes/u3_to_rphi.hpp"

#include <cmath>
#include <llvm/IR/Constants.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <vector>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#ifndef M_PI_2
#define M_PI_2 (M_PI / 2.0)
#endif

namespace qllvm {
namespace sabre {

namespace {

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

llvm::Function* getOrCreateRphi(llvm::Module* M) {
  auto* F = M->getFunction("__quantum__qis__rphi");
  if (F) return F;
  auto* T = llvm::Type::getVoidTy(M->getContext());
  auto* DblT = llvm::Type::getDoubleTy(M->getContext());
  auto* PtrT = llvm::Type::getInt8PtrTy(M->getContext());
  auto* FT = llvm::FunctionType::get(T, {DblT, DblT, PtrT}, false);
  return llvm::Function::Create(FT, llvm::Function::ExternalLinkage,
                                "__quantum__qis__rphi", M);
}

// U(theta,phi,lambda) = RZ(phi+lambda-theta) RPHI(M_PI_2,-1*(M_PI_2+lambda-theta)) RPHI(M_PI_2,-lambda+M_PI_2)
void trans_u3_to_rz_rphi(llvm::Module* M, llvm::CallInst* CI) {
  llvm::Value* theta = CI->getArgOperand(0);
  llvm::Value* phi = CI->getArgOperand(1);
  llvm::Value* lambda = CI->getArgOperand(2);
  llvm::Value* qubit = CI->getArgOperand(3);

  llvm::IRBuilder<> builder(CI);
  auto* rzFn = getOrCreateRz(M);
  auto* rphiFn = getOrCreateRphi(M);
  llvm::Type* dblTy = llvm::Type::getDoubleTy(M->getContext());
  llvm::Value* pi2 = llvm::ConstantFP::get(dblTy, M_PI_2);

  llvm::Value* rzAngle = builder.CreateFSub(builder.CreateFAdd(phi, lambda), theta);
  llvm::Value* rphi1Second =
      builder.CreateFSub(builder.CreateFSub(theta, lambda),pi2);
  llvm::Value* rphi2Second = builder.CreateFSub(pi2,lambda);
  
  // llvm::Value* rphi1Second =builder.CreateFSub(builder.CreateFAdd(pi2, lambda),theta);
  // llvm::Value* rphi2Second = builder.CreateFSub(lambda,pi2);

  builder.CreateCall(rzFn, {rzAngle, qubit});
  builder.CreateCall(rphiFn, {pi2, rphi1Second, qubit});
  builder.CreateCall(rphiFn, {pi2, rphi2Second, qubit});
  CI->eraseFromParent();
}


// U(theta,phi,lambda) = RZ(phi) RPHI(M_PI_2,pi) RZ(theta) RPHI(M_PI_2,0) RZ(lambda)
// void trans_u3_to_rz_rphi(llvm::Module* M, llvm::CallInst* CI) {
//   llvm::Value* theta = CI->getArgOperand(0);
//   llvm::Value* phi = CI->getArgOperand(1);
//   llvm::Value* lambda = CI->getArgOperand(2);
//   llvm::Value* qubit = CI->getArgOperand(3);

//   llvm::IRBuilder<> builder(CI);
//   auto* rzFn = getOrCreateRz(M);
//   auto* rphiFn = getOrCreateRphi(M);
//   llvm::Type* dblTy = llvm::Type::getDoubleTy(M->getContext());

//   llvm::Value* pi2 = llvm::ConstantFP::get(dblTy, M_PI_2);
//   llvm::Value* pi = llvm::ConstantFP::get(dblTy, M_PI);
//   llvm::Value* zero = llvm::ConstantFP::get(dblTy, 0.0);

//   llvm::Value* rzAngle = builder.CreateFSub(builder.CreateFAdd(phi, lambda), theta);
//   // llvm::Value* rphi1Second =
//   //     builder.CreateFSub(builder.CreateFSub(theta, lambda),pi2);
//   // llvm::Value* rphi2Second = builder.CreateFSub(pi2,lambda);
  
//   llvm::Value* rphi1Second =builder.CreateFSub(builder.CreateFAdd(pi2, lambda),theta);
//   llvm::Value* rphi2Second = builder.CreateFSub(lambda,pi2);

//   builder.CreateCall(rzFn, {phi, qubit});
//   builder.CreateCall(rphiFn, {pi2, pi, qubit});
//   builder.CreateCall(rzFn, {theta, qubit});
//   builder.CreateCall(rphiFn, {pi2, zero, qubit});
//   builder.CreateCall(rzFn, {lambda, qubit});

//   CI->eraseFromParent();
// }

}  // namespace

bool u3_to_rphi(llvm::Module* module) {
  if (!module) return true;
  std::vector<llvm::CallInst*> toProcess;
  for (auto& F : *module) {
    for (auto& BB : F) {
      for (auto& I : BB) {
        auto* CI = llvm::dyn_cast<llvm::CallInst>(&I);
        if (!CI || !CI->getCalledFunction()) continue;
        if (CI->getCalledFunction()->getName() != "__quantum__qis__u3")
          continue;
        toProcess.push_back(CI);
      }
    }
  }


  for (auto* CI : toProcess) {
    trans_u3_to_rz_rphi(module, CI);
  }
  return true;
}

}  // namespace sabre
}  // namespace qllvm
