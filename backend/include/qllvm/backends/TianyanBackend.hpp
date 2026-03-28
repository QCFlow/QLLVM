/*******************************************************************************
 * Tianyan backend: QIR -> Qcis
 *******************************************************************************/
#pragma once

#include "qllvm/Backend.hpp"

namespace qllvm {

class TianyanBackend : public Backend {
public:
  std::string name() const override { return "tianyan"; }
  bool emit(llvm::Module* module, const BackendOptions& opts) override;
};

}  // namespace qllvm
