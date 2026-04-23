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
 * CompilationPipeline - Orchestrates QASM -> MLIR -> LLVM IR -> output
 * Refactored from qllvm-compile for clearer separation of concerns.
 *******************************************************************************/
#pragma once

#include "qllvm/passes/SabreOptions.hpp"
#include <map>
#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

namespace qllvm {

/// Options for the compilation pipeline (populated from CLI or API)
struct CompilationOptions {
  std::string inputFile;
  std::string kernelName;
  bool addEntryPoint = true;
  int optLevel = 0;  // 0 = O0, 1 = O1
  bool qOptimizations = false;

  // Output control
  bool emitMLIR = false;
  bool emitQIR = false;
  std::string emitBackend;   // e.g. "qasm-backend"
  std::string outputPath;    // for backend emit
  std::string outputLlPath;  // for .ll file (default: stem + ".ll")

  // Hybrid compilation: classical C++ sources to link with quantum QIR
  std::vector<std::string> classicalSources;
  std::string outputExe;    // final executable path for hybrid build

  // Extra args passed to mlir_gen (qpu, qrt, shots, placement, etc.)
  std::map<std::string, std::string> extraArgs;

  // Optimization tuning (optional)
  std::optional<std::string> customPassSequenceFile;
  std::optional<std::unordered_set<std::string>> basicGateSet;
  bool circuitState = false;
  bool passCount = false;
  bool passEffect = false;
  bool synOpt = false;
  bool randomSeq = false;

  // SABRE qubit mapping (LLVM IR stage)
  std::optional<sabre::SabreOptions> sabreOptions;
};

/// Result of pipeline run
struct CompilationResult {
  int exitCode = 0;  // 0 = success
  std::string errorMessage;
  std::string emittedLlPath;  // path to .ll if written
  std::string emittedExePath;  // path to executable if hybrid build
};

/// Run the full compilation pipeline: QASM -> MLIR -> optimize -> LLVM IR -> output
/// @return exit code (0 = success, non-zero = failure)
CompilationResult runCompilationPipeline(const CompilationOptions& opts);

}  // namespace qllvm
