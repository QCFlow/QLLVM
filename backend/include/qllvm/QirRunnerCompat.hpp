/*******************************************************************************
 * QirRunnerCompat: Adapt qllvm QIR output for qir-alliance/qir-runner
 *
 * qllvm uses __quantum__rt__initialize(int, int8_t**).
 * qir-runner expects QIR base profile: void __quantum__rt__initialize(i8*).
 *
 * This transform:
 * 1. Replaces __quantum__rt__initialize signature to (i8*) -> void
 * 2. Creates EntryPoint() with "EntryPoint" attr for qir-runner
 *******************************************************************************/
#pragma once

#include <string>

namespace llvm {
class Module;
}

namespace qllvm {

/// Adapt LLVM module for qir-runner compatibility.
/// @param module The QIR module (modified in-place)
/// @param kernelName Hint for __internal_mlir_<name>, empty = auto-detect
/// @return true on success
bool adaptModuleForQirRunner(llvm::Module* module,
                             const std::string& kernelName = "");

}  // namespace qllvm
