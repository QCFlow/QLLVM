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
 * QIR Runner backend: QIR -> LLVM Bitcode (.bc)
 * Output format consumable by QIR runner / simulator (e.g. qir-alliance/qir-runner)
 *******************************************************************************/
#pragma once

#include "qllvm/Backend.hpp"

namespace qllvm {

class QirRunnerBackend : public Backend {
public:
  std::string name() const override { return "qir-runner"; }
  bool emit(llvm::Module* module, const BackendOptions& opts) override;
};

}  // namespace qllvm
