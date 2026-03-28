/*******************************************************************************
 * SabrePass: LLVM New PM ModulePass for SABRE qubit mapping
 *******************************************************************************/
#pragma once

#include "qllvm/passes/SabreOptions.hpp"
#include <llvm/IR/PassManager.h>

namespace llvm {
class Module;
}

namespace qllvm {
namespace sabre {

struct SabreOptions;

class SabrePass : public llvm::PassInfoMixin<SabrePass> {
public:
  explicit SabrePass(SabreOptions opts) : opts_(std::move(opts)) {}

  llvm::PreservedAnalyses run(llvm::Module& M,
                              llvm::ModuleAnalysisManager& AM);

  static bool isRequired() { return false; }

private:
  SabreOptions opts_;
};

bool runSabre(llvm::Module* module, const SabreOptions& opts,
              const std::string& kernelName = "");

}  // namespace sabre
}  // namespace qllvm
