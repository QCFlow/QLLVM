/*******************************************************************************
 * CircuitExtractor: QIR kernel -> gate list + DAG
 *******************************************************************************/
#pragma once

#include <llvm/IR/Instructions.h>
#include <vector>

namespace llvm {
class Function;
}

namespace qllvm {
namespace sabre {

enum class GateKind {
  H, X, Y, Z, S, Sdg, T, Tdg, Rx, Ry, Rz, P, U3, Sx,
  X2P, X2M, Y2P,
  Cnot, Cz, Swap, Cp, Mz, Reset,
  Unknown
};

struct Gate {
  GateKind kind = GateKind::Unknown;
  std::vector<int> qubits;
  std::vector<double> params;
  llvm::CallInst* callInst = nullptr;  // for IR modification (Strategy B)
  std::vector<llvm::Value*> operands;  // Qubit* Values, same order as qubits
};

struct Circuit {
  /// Logical wire count for SABRE: max(active qubits, allocate_array size when padded).
  int numQubits = 0;
  std::vector<Gate> gates;
  std::vector<std::vector<int>> successors;
  llvm::Value* qubitArrayAlloc = nullptr;  // For creating qubit loads when needed
};

class CircuitExtractor {
public:
  bool extract(llvm::Function* kernel, Circuit& out);
};

}  // namespace sabre
}  // namespace qllvm
