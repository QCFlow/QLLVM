#pragma once

#include <unordered_set>
#include <string>

namespace llvm {
class Module;
}

namespace qllvm {
namespace sabre {

// Decompose __quantum__qis__swap in QIR into basis gates.

bool runDecomposeSwapQir(
    llvm::Module* module,
    const std::unordered_set<std::string>& basicGateSet = {});

}  // namespace sabre
}  // namespace qllvm