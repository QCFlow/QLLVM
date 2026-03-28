/*******************************************************************************
 * QASM backend: QIR -> OpenQASM
 *******************************************************************************/
#pragma once

#include "qllvm/Backend.hpp"

namespace qllvm {

  class OriginQuantumBackend : public Backend {
    public:
      std::string name() const override { return "originquantum"; }
      bool emit(llvm::Module* module, const BackendOptions& opts) override;
    };

}  // namespace qllvm
