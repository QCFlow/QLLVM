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
