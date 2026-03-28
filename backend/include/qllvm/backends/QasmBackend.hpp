/*******************************************************************************
 * QASM backend: QIR -> OpenQASM
 *******************************************************************************/
#pragma once

#include "qllvm/Backend.hpp"

namespace qllvm {

class QasmBackend : public Backend {
public:
  std::string name() const override { return "qasm-backend"; }
  bool emit(llvm::Module* module, const BackendOptions& opts) override;
};

}  // namespace qllvm
