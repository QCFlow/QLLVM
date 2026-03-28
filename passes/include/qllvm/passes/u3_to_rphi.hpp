#pragma once

#include <unordered_set>
#include <string>

namespace llvm {
class Module;
}

namespace qllvm {
namespace sabre {

// Decompose __quantum__qis__u3 in QIR into basis gates.

bool u3_to_rphi( llvm::Module* module);

}  // namespace sabre
}  // namespace qllvm