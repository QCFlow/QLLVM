/*******************************************************************************
 * Backend abstract interface for QIR code generation
 *******************************************************************************/
#pragma once

#include "BackendOptions.hpp"
#include <string>

namespace llvm {
class Module;
}

namespace qllvm {

/// Backend abstract interface: QIR -> target code
class Backend {
public:
  virtual ~Backend() = default;

  /// Backend name, used for -qpu <name> matching
  virtual std::string name() const = 0;

  /// Convert QIR (llvm::Module) to target code
  /// @return true on success, false on failure
  virtual bool emit(llvm::Module* module, const BackendOptions& opts) = 0;
};

}  // namespace qllvm
