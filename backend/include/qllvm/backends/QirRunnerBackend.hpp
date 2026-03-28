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
