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
 * SabreSwap: SWAP-based routing for coupling map
 *******************************************************************************/
#pragma once

#include "qllvm/passes/CircuitExtractor.hpp"
#include "qllvm/passes/CouplingMap.hpp"
#include "qllvm/passes/SabreOptions.hpp"
#include <llvm/IR/Value.h>
#include <vector>

namespace llvm {
class CallInst;
class Value;
}

namespace qllvm {
namespace sabre {

struct SwapInsertion {
  int beforeGateIndex;
  int v0, v1;
  llvm::Value* val0 = nullptr;
  llvm::Value* val1 = nullptr;
  /// layout[logical] -> physical immediately before this swap (simulation order).
  std::vector<int> layoutBeforeSwap;
  /// Filled by SabrePass after inserting the swap call (for layout application).
  llvm::CallInst* swapCall = nullptr;
};

struct SabreSwapResult {
  std::vector<SwapInsertion> insertions;
  std::vector<int> finalLayout;
  /// layout[v] = physical index for logical v, captured immediately before each
  /// gate executes. Simulation schedules the earliest ready gate only so this
  /// matches linear IR order in applyPhysicalLayoutQir.
  std::vector<std::vector<int>> layoutBeforeGate;
};

class SabreSwap {
public:
  SabreSwap(const CouplingMap& coupling, const SabreOptions& opts);
  SabreSwapResult run(const Circuit& circ, const std::vector<int>& initialLayout);

private:
  const CouplingMap& coupling_;
  SabreOptions opts_;
  double scoreSwap(int v0, int v1, const std::vector<int>& layout,
                  const std::vector<int>& frontLayerGates,
                  const Circuit& circ);
};

}  // namespace sabre
}  // namespace qllvm
