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
 *
 * Modified by QCFlow (2026) for QLLVM project.
 */

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
