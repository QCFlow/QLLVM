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
