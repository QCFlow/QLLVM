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
