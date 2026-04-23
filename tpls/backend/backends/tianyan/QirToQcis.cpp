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
 * QIR to OpenQASM translator - Part of qllvm backend module
 *******************************************************************************/

 #include "QirToQcis.hpp"

 #include <llvm/IR/Constants.h>
 #include <llvm/IR/Function.h>
 #include <llvm/IR/Instructions.h>
 #include <llvm/IR/LLVMContext.h>
 #include <llvm/IR/Module.h>
 #include <llvm/Support/raw_ostream.h>
 
 #include <fstream>
 #include <iomanip>
 #include <sstream>
 #include <iostream>
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
 
          // After SABRE / applyPhysicalLayoutQir, operands may be bare GEPs or
          // BitCast(GEP) without going through Load; map every common form.
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
  // QCIS Table 2 constants
  constexpr double kPi = 3.141592653589793238462643383279502884;
  constexpr double kHalfPi = kPi / 2.0;


  auto emit_x = [&](int q){
    if (q >= 0) out << "X2P Q" << q << "\n" << "X2P Q" << q << "\n";
  };
  auto emit_y = [&](int q){
    if (q >= 0) out << "Y2P Q" << q << "\n" << "Y2P Q" << q << "\n";
  };
  auto emit_z = [&](int q){
    if (q >= 0) out << "RZ Q" << q << " " << std::setprecision(17) << kPi << "\n";
  };
  auto emit_s = [&](int q){
    if (q >= 0) out << "RZ Q" << q << " " << std::setprecision(17) << kHalfPi << "\n";
  };
  auto emit_sdg = [&](int q){
    if (q >= 0) out << "RZ Q" << q << " " << std::setprecision(17) << -1*kHalfPi << "\n";
  };
  auto emit_t = [&](int q){
    if (q >= 0) out << "RZ Q" << q << " " << std::setprecision(17) << kHalfPi /2.0  << "\n";
  };   
  auto emit_tdg = [&](int q){
    if (q >= 0) out << "RZ Q" << q << " " << std::setprecision(17) << -1* kHalfPi /2.0  << "\n";
  };


  auto emit_h = [&](int q) {
    if (q >= 0) out << "RZ Q" << q << " " << std::setprecision(17) << kPi << " \n"
                      << "Y2P Q" << q << "\n";
  };

  auto emit_rx = [&](double theta, int q){
    if (q >= 0) out << "RZ Q" << q << " " << std::setprecision(17) << kHalfPi << " \n"
                    << "X2P Q" << q << "\n"
                    << "RZ Q" << q << " "  << std::setprecision(17) << theta << "\n"
                    << "X2M Q" << q << "\n"
                    << "RZ Q" << q << " "  << std::setprecision(17) << -1 * kHalfPi << "\n";
  };
  auto emit_ry = [&](double theta, int q){
    if (q >= 0) out << "X2P Q" << q << "\n"
                  << "RZ Q" << q << " " << theta << "\n"
                  << "X2M Q" << q << "\n";
  };
  auto emit_rz = [&](int q, double theta) {
    if (q >= 0) out << "RZ Q" << q << " " << std::setprecision(17) << theta << "\n";
  };

  auto emit_rxy = [&](double phi, double theta, int q) {
    if (q >= 0)
      out << "RZ Q" << q << " " << std::setprecision(17) << (kHalfPi - phi) << "\n"
          << "X2P Q" << q << "\n"
          << "RZ Q" << q << " " << std::setprecision(17) << theta << "\n"
          << "X2M Q" << q << "\n"
          << "RZ Q" << q << " " << std::setprecision(17) << (phi - kHalfPi) << "\n";
  };

  auto emit_cx = [&](int ctrl,int tgt){
   if (ctrl >= 0 && tgt >= 0) out << "Y2M Q" << tgt << "\n"
                                  << "CZ Q" << ctrl << " Q" << tgt << "\n"
                                  << "Y2P Q" << tgt << "\n";
  };
  auto emit_cy = [&](int ctrl,int tgt){
    if (ctrl >= 0 && tgt >= 0) out << "RZ Q" << tgt << " " << std::setprecision(17) << kHalfPi << "\n"
                                   << "Y2P Q" << tgt << "\n"
                                   << "CZ Q" << ctrl << " Q" << tgt << "\n"
                                   << "Y2M Q" << tgt << "\n"
                                   << "RZ Q" << tgt << " " << std::setprecision(17) << -1*kHalfPi << "\n";
  };
  auto emit_cz = [&](int q0, int q1) {
    if (q0 >= 0 && q1 >= 0) out << "CZ Q" << q0 << " Q" << q1 << "\n";
  };


  if (name == "__quantum__qis__h" || name == "__quantum__qis__H") {
    int q = getQubitIndex(CI->getArgOperand(0), qubitMap);
    emit_h(q);
    return q >= 0;
  }
  if (name == "__quantum__qis__x" || name == "__quantum__qis__X") {
    int q = getQubitIndex(CI->getArgOperand(0), qubitMap);
    emit_x(q);
    return q >= 0;
  }
  if (name == "__quantum__qis__y" || name == "__quantum__qis__Y") {
    int q = getQubitIndex(CI->getArgOperand(0), qubitMap);
    emit_y(q);
    return q >= 0;
  }
  if (name == "__quantum__qis__z" || name == "__quantum__qis__Z") {
    int q = getQubitIndex(CI->getArgOperand(0), qubitMap);
    emit_z(q);
    return q >= 0;
  }

  if (name == "__quantum__qis__s" || name == "__quantum__qis__S") {
    emit_s(getQubitIndex(CI->getArgOperand(0), qubitMap));
    return true;
  }

  if (name == "__quantum__qis__sdg" || name == "__quantum__qis__SDG") {
    emit_sdg(getQubitIndex(CI->getArgOperand(0), qubitMap));
    return true;
  }
  if (name == "__quantum__qis__t" || name == "__quantum__qis__T") {
    emit_t(getQubitIndex(CI->getArgOperand(0), qubitMap));
    return true;
  }
  if (name == "__quantum__qis__tdg" || name == "__quantum__qis__TDG") {
    emit_tdg(getQubitIndex(CI->getArgOperand(0), qubitMap));
    return true;
  }
  if (name == "__quantum__qis__rx" || name == "__quantum__qis__RX") {
    double theta = getConstantDouble(CI->getArgOperand(0));
    int q = getQubitIndex(CI->getArgOperand(1), qubitMap);
    emit_rx(theta, q);
    return q >= 0;
  }
  if (name == "__quantum__qis__ry" || name == "__quantum__qis__RY") {
    double theta = getConstantDouble(CI->getArgOperand(0));
    int q = getQubitIndex(CI->getArgOperand(1), qubitMap);
    emit_ry(theta, q);
    return q >= 0;
  }
  if (name == "__quantum__qis__rz" || name == "__quantum__qis__RZ") {
    double theta = getConstantDouble(CI->getArgOperand(0));
    int q = getQubitIndex(CI->getArgOperand(1), qubitMap);
    emit_rz(q, theta);
    return q >= 0;
  }

  if (name == "__quantum__qis__rxy" || name == "__quantum__qis__RXY") {
    // Expected signature: rxy(phi, theta, qubit)
    double phi = getConstantDouble(CI->getArgOperand(0));
    double theta = getConstantDouble(CI->getArgOperand(1));
    int q = getQubitIndex(CI->getArgOperand(2), qubitMap);
    emit_rxy(phi, theta, q);
    return q >= 0;
  }


  if (name == "__quantum__qis__p" || name == "__quantum__qis__P") {
    double theta = getConstantDouble(CI->getArgOperand(0));
    int q = getQubitIndex(CI->getArgOperand(1), qubitMap);
    emit_rz(q, theta);
    return q >= 0;
  }
  
  if (name == "__quantum__qis__cnot") {
    int q0 = getQubitIndex(CI->getArgOperand(0), qubitMap);
    int q1 = getQubitIndex(CI->getArgOperand(1), qubitMap);
    emit_cx(q0, q1);
    return q0 >= 0 && q1 >= 0;
  }
  if (name == "__quantum__qis__cz" || name == "__quantum__qis__CZ") {
    int q0 = getQubitIndex(CI->getArgOperand(0), qubitMap);
    int q1 = getQubitIndex(CI->getArgOperand(1), qubitMap);
    emit_cz(q0, q1);
    return q0 >= 0 && q1 >= 0;
  }
  if (name == "__quantum__qis__cy" || name == "__quantum__qis__CY") {
    int q0 = getQubitIndex(CI->getArgOperand(0), qubitMap);
    int q1 = getQubitIndex(CI->getArgOperand(1), qubitMap);
    emit_cy(q0, q1);
    return q0 >= 0 && q1 >= 0;
  }


  if (name == "__quantum__qis__mz") {
    int q = getQubitIndex(CI->getArgOperand(0), qubitMap);
    if (q >= 0) {
      out << "M Q" << q << "\n";
      measureCount++;
    }
    return q >= 0;
  }

   return true;  // unknown gate, skip
 }
 
 }  // namespace
 
 std::string QirToQcisTranslator::translate(llvm::Module* module,
                                            const std::string& kernelName) {
   if (!module) return "";
 
   Function* Kernel = findKernelFunction(module, kernelName);
   if (!Kernel) return "";
 
   QubitMap qubitMap;
   int totalQubits = 0;
   if (!buildQubitMap(Kernel, qubitMap, totalQubits)) return "";
  (void)totalQubits;
 
   int measureCount = 0;
   std::ostringstream out;
  // QCIS output: one instruction per line.
  // Note: QCIS is a control instruction set, not OpenQASM.
   for (auto& BB : *Kernel) {
     for (auto& I : BB) {
       CallInst* CI = dyn_cast<CallInst>(&I);
       if (!CI) continue;
       emitInstruction(CI, qubitMap, out, measureCount);
     }
   }
  //  std::string result_t = out.str();
  //  printf("%s", result_t.c_str());
   return out.str();
 }
 
 bool QirToQcisTranslator::translateToFile(llvm::Module* module,
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
 
 std::string qirToQcis(llvm::Module* module, const std::string& kernelName) {
  
   QirToQcisTranslator translator;
   return translator.translate(module, kernelName);
 }
 
 bool qirToQcisFile(llvm::Module* module, const std::string& outPath,
                    const std::string& kernelName) {
   QirToQcisTranslator translator;
   return translator.translateToFile(module, outPath, kernelName);
 }
 
 }  // namespace qllvm
 