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
 * After SABRE: rewrite QIR so qubit operands use device physical indices
 * (GEP index into qubit array sized numPhys).
 *******************************************************************************/

#pragma once

#include "qllvm/passes/CircuitExtractor.hpp"
#include "qllvm/passes/SabreSwap.hpp"

#include <vector>

namespace llvm {
class Function;
class Value;
}

namespace qllvm {
namespace sabre {

/// Resize qubit array to numPhys and rewrite qubit operands by walking the
/// kernel in IR order: maintain layout[logical]->physical, updating on each
/// __quantum__qis__swap (Sabre routing or user swap) before rewriting that
/// instruction. This matches hardware execution order and avoids incorrect
/// fallbacks when per-gate snapshots are missing.
bool applyPhysicalLayoutQir(llvm::Function* kernel, llvm::Value* qubitAlloc,
                            llvm::Function* gepFn, int numPhys,
                            const std::vector<int>& initialLayout,
                            const Circuit& circ,
                            const SabreSwapResult& sabreResult);

}  // namespace sabre
}  // namespace qllvm
