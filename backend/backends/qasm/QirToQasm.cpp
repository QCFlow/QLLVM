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
 *
 * Modified by QCFlow (2026) for QLLVM project.
 */

/*******************************************************************************
 * QIR to OpenQASM translator - Part of qllvm backend module
 *******************************************************************************/

#include "QirToQasm.hpp"

#include <llvm/IR/Constants.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/raw_ostream.h>

#include <fstream>
#include <iomanip>
#include <sstream>
#include <unordered_map>
#include <vector>

using namespace llvm;

namespace qllvm {

namespace {

// Map Value* (Qubit* SSA) -> global qubit index
using QubitMap = std::unordered_map<const Value*, int>;

// Find kernel function: __internal_mlir_<name> or first __internal_mlir_*
Function* findKernelFunction(Module* M, const std::string& kernelName) {
  Function* found = nullptr;
  for (auto& F : *M) {
    if (F.isDeclaration()) continue;
    StringRef name = F.getName();
    if (name.startswith("__internal_mlir_")) {
      if (!kernelName.empty()) {
        std::string expected = "__internal_mlir_" + kernelName;
        if (name == expected) return &F;
      }
      if (!found) found = &F;
    }
  }
  return found;
}

// Resolve qubit index from Value* (may be through bitcast)
const Value* traceToQubitValue(const Value* V) {
  if (!V) return nullptr;
  // BitCast: i8* -> Qubit**, the load uses the result
  if (const BitCastInst* BC = dyn_cast<BitCastInst>(V))
    return traceToQubitValue(BC->getOperand(0));
  return V;
}

// Build qubit map from kernel function
bool buildQubitMap(Function* F, QubitMap& qubitMap, int& totalQubits) {
  totalQubits = 0;
  qubitMap.clear();

  for (auto& BB : *F) {
    for (auto& I : BB) {
      CallInst* CI = dyn_cast<CallInst>(&I);
      if (!CI) continue;

      Function* Callee = CI->getCalledFunction();
      if (!Callee) continue;

      StringRef name = Callee->getName();

      if (name == "__quantum__rt__qubit_allocate_array") {
        if (CI->getNumArgOperands() < 1) return false;
        const Value* NVal = CI->getArgOperand(0);
        const ConstantInt* CInt = dyn_cast<ConstantInt>(NVal);
        if (!CInt) return false;
        int N = (int)CInt->getZExtValue();
        // Array %array; qubits will be 0..N-1 for this array
        // We need to map: get_element_ptr_1d(%array, idx) -> totalQubits + idx
        int base = totalQubits;
        totalQubits += N;

        // Scan forward to find get_element_ptr_1d uses of this array
        for (auto U : CI->users()) {
          CallInst* GEPCall = dyn_cast<CallInst>(U);
          if (!GEPCall || !GEPCall->getCalledFunction()) continue;
          if (GEPCall->getCalledFunction()->getName() !=
              "__quantum__rt__array_get_element_ptr_1d")
            continue;
          if (GEPCall->getNumArgOperands() < 2) continue;
          if (GEPCall->getArgOperand(0) != CI) continue;
          const Value* IdxVal = GEPCall->getArgOperand(1);
          const ConstantInt* IdxC = dyn_cast<ConstantInt>(IdxVal);
          if (!IdxC) continue;
          int idx = (int)IdxC->getZExtValue();
          int qidx = base + idx;

          qubitMap[GEPCall] = qidx;
          for (auto GEPU : GEPCall->users()) {
            if (BitCastInst* BC = dyn_cast<BitCastInst>(GEPU)) {
              qubitMap[BC] = qidx;
              for (auto BCU : BC->users()) {
                if (LoadInst* LI = dyn_cast<LoadInst>(BCU))
                  qubitMap[LI] = qidx;
              }
            } else if (LoadInst* LI = dyn_cast<LoadInst>(GEPU)) {
              qubitMap[LI] = qidx;
            }
          }
        }
      }
    }
  }
  return true;
}

// Get qubit index from Value*, return -1 if not found
int getQubitIndex(const Value* V, const QubitMap& qubitMap) {
  if (!V) return -1;
  auto it = qubitMap.find(V);
  if (it != qubitMap.end()) return it->second;
  // Handle phi/select by checking operand (simplified)
  return -1;
}

// Extract double from ConstantFP or return 0
double getConstantDouble(const Value* V) {
  if (const ConstantFP* CF = dyn_cast<ConstantFP>(V))
    return CF->getValueAPF().convertToDouble();
  return 0.0;
}

// Emit gate/measure to string stream
bool emitInstruction(CallInst* CI, const QubitMap& qubitMap,
                    std::ostream& out, int& measureCount) {
  Function* Callee = CI->getCalledFunction();
  if (!Callee) return true;  // skip indirect calls
  StringRef name = Callee->getName();

  if (!name.startswith("__quantum__qis__")) return true;

  auto emit1q = [&](const std::string& gate, int q) {
    if (q >= 0) out << gate << " q[" << q << "];\n";
  };
  auto emit1qParam = [&](const std::string& gate, double theta, int q) {
    if (q >= 0)
      out << gate << "(" << std::setprecision(17)  << theta << ") q[" << q
          << "];\n";
  };
  auto emit2q = [&](const std::string& gate, int q0, int q1) {
    if (q0 >= 0 && q1 >= 0)
      out << gate << " q[" << q0 << "], q[" << q1 << "];\n";
  };
  auto emit2qParam = [&](const std::string& gate, double theta, int q0, int q1) {
    if (q0 >= 0 && q1 >= 0)
      out << gate << "(" << std::setprecision(17) << theta << ") q[" << q0
          << "], q[" << q1 << "];\n";
  };
  auto emit3qParam = [&](double t, double p, double l, int q) {
    if (q >= 0)
      out << "u3(" << std::setprecision(17) << t << "," << p << "," << l
          << ") q[" << q << "];\n";
  };

  if (name == "__quantum__qis__h") {
    int q = getQubitIndex(CI->getArgOperand(0), qubitMap);
    emit1q("h", q);
    return q >= 0;
  }
  if (name == "__quantum__qis__x" || name == "__quantum__qis__y" ||
      name == "__quantum__qis__z") {
    int q = getQubitIndex(CI->getArgOperand(0), qubitMap);
    char c = name.back();
    emit1q(std::string(1, c), q);
    return q >= 0;
  }
  if (name == "__quantum__qis__s") {
    emit1q("s", getQubitIndex(CI->getArgOperand(0), qubitMap));
    return true;
  }
  if (name == "__quantum__qis__sx") {
    emit1q("sx", getQubitIndex(CI->getArgOperand(0), qubitMap));
    return true;
  }
  if (name == "__quantum__qis__sdg") {
    emit1q("sdg", getQubitIndex(CI->getArgOperand(0), qubitMap));
    return true;
  }
  if (name == "__quantum__qis__t") {
    emit1q("t", getQubitIndex(CI->getArgOperand(0), qubitMap));
    return true;
  }
  if (name == "__quantum__qis__tdg") {
    emit1q("tdg", getQubitIndex(CI->getArgOperand(0), qubitMap));
    return true;
  }
  if (name == "__quantum__qis__rx") {
    double theta = getConstantDouble(CI->getArgOperand(0));
    int q = getQubitIndex(CI->getArgOperand(1), qubitMap);
    emit1qParam("rx", theta, q);
    return q >= 0;
  }
  if (name == "__quantum__qis__ry") {
    double theta = getConstantDouble(CI->getArgOperand(0));
    int q = getQubitIndex(CI->getArgOperand(1), qubitMap);
    emit1qParam("ry", theta, q);
    return q >= 0;
  }
  if (name == "__quantum__qis__rz") {
    double theta = getConstantDouble(CI->getArgOperand(0));
    int q = getQubitIndex(CI->getArgOperand(1), qubitMap);
    emit1qParam("rz", theta, q);
    return q >= 0;
  }
  if (name == "__quantum__qis__p") {
    double theta = getConstantDouble(CI->getArgOperand(0));
    int q = getQubitIndex(CI->getArgOperand(1), qubitMap);
    emit1qParam("p", theta, q);
    return q >= 0;
  }
  if (name == "__quantum__qis__u3") {
    double theta = getConstantDouble(CI->getArgOperand(0));
    double phi = getConstantDouble(CI->getArgOperand(1));
    double lambda = getConstantDouble(CI->getArgOperand(2));
    int q = getQubitIndex(CI->getArgOperand(3), qubitMap);
    emit3qParam(theta, phi, lambda, q);
    return q >= 0;
  }
  if (name == "__quantum__qis__su2") {
    double theta = getConstantDouble(CI->getArgOperand(0));
    double lambda = getConstantDouble(CI->getArgOperand(1));
    double phi = getConstantDouble(CI->getArgOperand(2));
    int q = getQubitIndex(CI->getArgOperand(3), qubitMap);
    emit3qParam(theta, phi, lambda, q);
    return q >= 0;
  }
  if (name == "__quantum__qis__cnot") {
    int q0 = getQubitIndex(CI->getArgOperand(0), qubitMap);
    int q1 = getQubitIndex(CI->getArgOperand(1), qubitMap);
    emit2q("cx", q0, q1);
    return q0 >= 0 && q1 >= 0;
  }
  if (name == "__quantum__qis__cz") {
    int q0 = getQubitIndex(CI->getArgOperand(0), qubitMap);
    int q1 = getQubitIndex(CI->getArgOperand(1), qubitMap);
    emit2q("cz", q0, q1);
    return q0 >= 0 && q1 >= 0;
  }
  if (name == "__quantum__qis__swap") {
    int q0 = getQubitIndex(CI->getArgOperand(0), qubitMap);
    int q1 = getQubitIndex(CI->getArgOperand(1), qubitMap);
    emit2q("swap", q0, q1);
    return q0 >= 0 && q1 >= 0;
  }
  if (name == "__quantum__qis__cp" || name == "__quantum__qis__cphase") {
    double theta = getConstantDouble(CI->getArgOperand(0));
    int q0 = getQubitIndex(CI->getArgOperand(1), qubitMap);
    int q1 = getQubitIndex(CI->getArgOperand(2), qubitMap);
    emit2qParam("cp", theta, q0, q1);
    return q0 >= 0 && q1 >= 0;
  }
  if (name == "__quantum__qis__mz") {
    int q = getQubitIndex(CI->getArgOperand(0), qubitMap);
    if (q >= 0) {
      out << "measure q[" << q << "] -> c[" << measureCount << "];\n";
      measureCount++;
    }
    return q >= 0;
  }
  if (name == "__quantum__qis__reset") {
    int q = getQubitIndex(CI->getArgOperand(0), qubitMap);
    emit1q("reset", q);
    return q >= 0;
  }

  return true;  // unknown gate, skip
}

}  // namespace

std::string QirToQasmTranslator::translate(llvm::Module* module,
                                           const std::string& kernelName) {
  if (!module) return "";

  Function* Kernel = findKernelFunction(module, kernelName);
  if (!Kernel) return "";

  QubitMap qubitMap;
  int totalQubits = 0;
  if (!buildQubitMap(Kernel, qubitMap, totalQubits)) return "";

  int measureCount = 0;
  std::ostringstream out;

  out << "OPENQASM 2.0;\n";
  out << "include \"qelib1.inc\";\n";
  out << "qreg q[" << totalQubits << "];\n";
  for (auto& BB : *Kernel) {
    for (auto& I : BB) {
      CallInst* CI = dyn_cast<CallInst>(&I);
      if (!CI || !CI->getCalledFunction()) continue;
      if (CI->getCalledFunction()->getName() == "__quantum__qis__mz")
        measureCount++;
    }
  }

  out << "creg c[" << measureCount << "];\n";

  measureCount = 0;
  for (auto& BB : *Kernel) {
    for (auto& I : BB) {
      CallInst* CI = dyn_cast<CallInst>(&I);
      if (!CI) continue;
      emitInstruction(CI, qubitMap, out, measureCount);
    }
  }

  return out.str();
}

bool QirToQasmTranslator::translateToFile(llvm::Module* module,
                                          const std::string& outPath,
                                          const std::string& kernelName) {
  std::string qasm = translate(module, kernelName);
  if (qasm.empty()) return false;

  std::ofstream ofs(outPath);
  if (!ofs) return false;
  ofs << qasm;
  ofs.close();
  return true;
}

std::string qirToQasm(llvm::Module* module, const std::string& kernelName) {
  QirToQasmTranslator translator;
  return translator.translate(module, kernelName);
}

bool qirToQasmFile(llvm::Module* module, const std::string& outPath,
                   const std::string& kernelName) {
  QirToQasmTranslator translator;
  return translator.translateToFile(module, outPath, kernelName);
}

}  // namespace qllvm
