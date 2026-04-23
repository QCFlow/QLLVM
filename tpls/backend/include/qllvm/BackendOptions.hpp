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
 * Backend options for QIR code generation
 *******************************************************************************/
#pragma once

#include <string>

namespace qllvm {

/// Options passed to Backend::emit
struct BackendOptions {
  std::string outputPath;
  std::string kernelName;
  std::string format;  // "openqasm2", "openqasm3", "quil", ...
  std::string shots;
};

}  // namespace qllvm
