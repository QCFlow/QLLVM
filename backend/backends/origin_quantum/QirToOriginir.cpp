/*******************************************************************************
 * QIR to OpenQASM translator - Part of qllvm backend module
 *******************************************************************************/

 #include "QirToOriginir.hpp"

 #include <llvm/IR/Constants.h>
 #include <llvm/IR/Function.h>
 #include <llvm/IR/Instructions.h>
 #include <llvm/IR/LLVMContext.h>
 #include <llvm/IR/Module.h>
 #include <llvm/Support/raw_ostream.h>
 
 #include <fstream>
 #include <iomanip>
 #include <random>
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
 bool all_close(double a, double b){
  if(std::abs(a - b) < 1e-6){
      return true;
  }else{
      return false;
  }
}
 
 // Emit gate/measure to string stream
 bool emitInstruction(CallInst* CI, const QubitMap& qubitMap,
                     std::ostream& out, int& measureCount) {
   Function* Callee = CI->getCalledFunction();
   if (!Callee) return true;  // skip indirect calls
   StringRef name = Callee->getName();
 
   if (!name.startswith("__quantum__qis__")) return true;
  // Originir Table 2 constants
  constexpr double kPi = 3.141592653589793238462643383279502884;
  constexpr double kHalfPi = kPi / 2.0;

  auto emit_x = [&](int q){
    if (q >= 0) out << "X q[" << q << "]\n";
  };
  auto emit_y = [&](int q){
    if (q >= 0) out << "Y q[" << q << "]\n";
  };
  auto emit_z = [&](int q){
    if (q >= 0) out << "Z q[" << q << "]\n";
  };
  auto emit_s = [&](int q){
    if (q >= 0) out << "S q[" << q << "]\n";
  };
  auto emit_t = [&](int q){
    if (q >= 0) out << "T q[" << q << "]\n";
  }; 
  auto emit_h = [&](int q){
    if (q >= 0) out << "H q[" << q << "]\n";
  };
  auto emit_sx = [&](int q){
    if (q >= 0) out << "X1 q[" << q << "]\n";
  };
  auto emit_rx = [&](double theta, int q){
    if (q >= 0){
      if(all_close(theta,kHalfPi)) {
        out << "X1 q[" << q << "]\n";
      } else {
        out << "RX q[" << q << "],("  << std::setprecision(17) << theta  << ")"<< "\n";
      }
    }
  };
  auto emit_ry = [&](double theta, int q){
    if (q >= 0){
      if(all_close(theta,kHalfPi)) {
        out << "Y1 q[" << q << "]\n";
      } else {
        out << "RY q[" << q << "],(" << std::setprecision(17) << theta  << ")"<< "\n";
      }
    }
  };
  auto emit_rz = [&](int q, double theta) {
    if (q >= 0){
      out << "RZ q[" << q << "],(" << std::setprecision(17) << theta  << ")"<< "\n";
    } 
  };
  auto emit_u3 = [&](int q, double theta, double phi, double lambda) {
    if (q >= 0) out << "U3 q[" << q << "],(" << std::setprecision(17) << theta  << "," << std::setprecision(17) << phi  << "," << std::setprecision(17) << lambda  << ")"<< "\n";
  };
  auto emit_u1 = [&](int q, double theta) {
    if (q >= 0) out << "U1 q[" << q << "],(" << std::setprecision(17) << theta  << ")"<< "\n";
  };
  auto emit_u2 = [&](int q, double phi, double lambda) {
    if (q >= 0) out << "U2 q[" << q << "],(" << std::setprecision(17) << phi  << "," << std::setprecision(17) << lambda  << ")"<< "\n";
  };
  auto emit_cx = [&](int ctrl,int tgt){
   if (ctrl >= 0 && tgt >= 0) out << "CNOT q[" << ctrl << "],q[" << tgt << "]\n";
  };
  auto emit_cz = [&](int q0, int q1) {
    if (q0 >= 0 && q1 >= 0) out << "CZ q[" << q0 << "],q[" << q1 << "]\n";
  };
  auto emit_swap = [&](int q0, int q1) {
    if (q0 >= 0 && q1 >= 0) out << "SWAP q[" << q0 << "],q[" << q1 << "]\n";
  };
  auto emit_ccx = [&](int ctrl1, int ctrl2, int tgt) {
    if (ctrl1 >= 0 && ctrl2 >= 0 && tgt >= 0) out << "TOFFOLI q[" << ctrl1 << "],q[" << ctrl2 << "],q[" << tgt << "]\n";
  };

  auto emit_cp = [&](int q0, int q1, double theta) {
    if (q0 >= 0 && q1 >= 0) out << "CR q[" << q0 << "],q[" << q1 << "],(" << std::setprecision(17) << theta  << ")"<< "\n";
  };
  auto emit_sdg = [&](int q){
    if (q >= 0) out << "DAGGER\n"
                    << "S q[" << q << "]\n"
                    << "ENDDAGGER\n";
  };
  auto emit_tdg = [&](int q){
    if (q >= 0) out << "DAGGER\n"
                    << "T q[" << q << "]\n"
                    << "ENDDAGGER\n";
  };
  auto emit_rxx = [&](int q0, int q1, double theta) {
    if (q0 >= 0 && q1 >= 0) out << "RXX q[" << q0 << "],q[" << q1 << "],(" << std::setprecision(17) << theta  << ")"<< "\n";
  };
  auto emit_ryy = [&](int q0, int q1, double theta) {
    if (q0 >= 0 && q1 >= 0) out << "RYY q[" << q0 << "],q[" << q1 << "],(" << std::setprecision(17) << theta  << ")"<< "\n";
  };
  auto emit_rzz = [&](int q0, int q1, double theta) {
    if (q0 >= 0 && q1 >= 0) out << "RZZ q[" << q0 << "],q[" << q1 << "],(" << std::setprecision(17) << theta  << ")"<< "\n";
  };
  auto emit_rzx = [&](int q0, int q1, double theta) {
    if (q0 >= 0 && q1 >= 0) out << "RZX q[" << q0 << "],q[" << q1 << "],(" << std::setprecision(17) << theta  << ")"<< "\n";
  };
  auto emit_rphi = [&](int q, double theta, double phi) {
    if (q >= 0) out << "RPHI q[" << q << "],(" << std::setprecision(17) << theta  << "," << std::setprecision(17) << phi  << ")"<< "\n";
  };

  if (name == "__quantum__qis__h") {
    int q = getQubitIndex(CI->getArgOperand(0), qubitMap);
    emit_h(q);
    return q >= 0;
  }
  if (name == "__quantum__qis__x") {
    int q = getQubitIndex(CI->getArgOperand(0), qubitMap);
    emit_x(q);
    return q >= 0;
  }
  if (name == "__quantum__qis__y") {
    int q = getQubitIndex(CI->getArgOperand(0), qubitMap);
    emit_y(q);
    return q >= 0;
  }
  if (name == "__quantum__qis__z") {
    int q = getQubitIndex(CI->getArgOperand(0), qubitMap);
    emit_z(q);
    return q >= 0;
  }
  if (name == "__quantum__qis__s") {
    emit_s(getQubitIndex(CI->getArgOperand(0), qubitMap));
    return true;
  }
  if (name == "__quantum__qis__sx") {
    emit_sx(getQubitIndex(CI->getArgOperand(0), qubitMap));
    return true;
  }
  if (name == "__quantum__qis__t") {
    emit_t(getQubitIndex(CI->getArgOperand(0), qubitMap));
    return true;
  }
  if (name == "__quantum__qis__rx") {
    double theta = getConstantDouble(CI->getArgOperand(0));
    int q = getQubitIndex(CI->getArgOperand(1), qubitMap);
    emit_rx(theta, q);
    return q >= 0;
  }
  if (name == "__quantum__qis__ry") {
    double theta = getConstantDouble(CI->getArgOperand(0));
    int q = getQubitIndex(CI->getArgOperand(1), qubitMap);
    emit_ry(theta, q);
    return q >= 0;
  }
  if (name == "__quantum__qis__rz" || name == "__quantum__qis__p") {
    double theta = getConstantDouble(CI->getArgOperand(0));
    int q = getQubitIndex(CI->getArgOperand(1), qubitMap);
    emit_rz(q, theta);
    return q >= 0;
  }
  if (name == "__quantum__qis__u3") {
    double theta = getConstantDouble(CI->getArgOperand(0));
    double phi = getConstantDouble(CI->getArgOperand(1));
    double lambda = getConstantDouble(CI->getArgOperand(2));
    int q = getQubitIndex(CI->getArgOperand(3), qubitMap);
    emit_u3(q, theta, phi, lambda);
    return q >= 0;
  }
  if (name == "__quantum__qis__rphi") {
    double theta = getConstantDouble(CI->getArgOperand(0));
    double phi = getConstantDouble(CI->getArgOperand(1));
    int q = getQubitIndex(CI->getArgOperand(2), qubitMap);
    emit_rphi(q, theta, phi);
    return q >= 0;
  }

  if (name == "__quantum__qis__cnot") {
    int q0 = getQubitIndex(CI->getArgOperand(0), qubitMap);
    int q1 = getQubitIndex(CI->getArgOperand(1), qubitMap);
    emit_cx(q0, q1);
    return q0 >= 0 && q1 >= 0;
  }
  if (name == "__quantum__qis__cz") {
    int q0 = getQubitIndex(CI->getArgOperand(0), qubitMap);
    int q1 = getQubitIndex(CI->getArgOperand(1), qubitMap);
    emit_cz(q0, q1);
    return q0 >= 0 && q1 >= 0;
  }
  if (name == "__quantum__qis__cp") {
    double theta = getConstantDouble(CI->getArgOperand(0));
    int q0 = getQubitIndex(CI->getArgOperand(1), qubitMap);
    int q1 = getQubitIndex(CI->getArgOperand(2), qubitMap);
    
    emit_cp(q0, q1, theta);
    return q0 >= 0 && q1 >= 0;
  }
  if (name == "__quantum__qis__swap") {
    int q0 = getQubitIndex(CI->getArgOperand(0), qubitMap);
    int q1 = getQubitIndex(CI->getArgOperand(1), qubitMap);
    emit_swap(q0, q1);
    return q0 >= 0 && q1 >= 0;
  }
  if (name == "__quantum__qis__sdg") {
    int q0 = getQubitIndex(CI->getArgOperand(0), qubitMap);
    emit_sdg(q0);
    return q0 >= 0;
  }
  if (name == "__quantum__qis__tdg") {
    int q0 = getQubitIndex(CI->getArgOperand(0), qubitMap);
    emit_tdg(q0);
    return q0 >= 0;
  }
  if (name == "__quantum__qis__ccx") {
    int ctrl1 = getQubitIndex(CI->getArgOperand(0), qubitMap);
    int ctrl2 = getQubitIndex(CI->getArgOperand(1), qubitMap);
    int tgt = getQubitIndex(CI->getArgOperand(2), qubitMap);
    emit_ccx(ctrl1, ctrl2, tgt);
    return ctrl1 >= 0 && ctrl2 >= 0 && tgt >= 0;
  }
  if (name == "__quantum__qis__rxx") {
    double theta = getConstantDouble(CI->getArgOperand(0));
    int q0 = getQubitIndex(CI->getArgOperand(1), qubitMap);
    int q1 = getQubitIndex(CI->getArgOperand(2), qubitMap);
    emit_rxx(q0, q1, theta);
    return q0 >= 0 && q1 >= 0;
  }
  if (name == "__quantum__qis__ryy") {
    double theta = getConstantDouble(CI->getArgOperand(0));
    int q0 = getQubitIndex(CI->getArgOperand(1), qubitMap);
    int q1 = getQubitIndex(CI->getArgOperand(2), qubitMap);
    emit_ryy(q0, q1, theta);
    return q0 >= 0 && q1 >= 0;
  }
  if (name == "__quantum__qis__rzz") {
    double theta = getConstantDouble(CI->getArgOperand(0));
    int q0 = getQubitIndex(CI->getArgOperand(1), qubitMap);
    int q1 = getQubitIndex(CI->getArgOperand(2), qubitMap);
    emit_rzz(q0, q1, theta);
    return q0 >= 0 && q1 >= 0;
  }
  

  if (name == "__quantum__qis__mz") {
    int q = getQubitIndex(CI->getArgOperand(0), qubitMap);
    if (q >= 0) {
      out << "MEASURE q[" << q << "],c[" << measureCount << "]\n";
      measureCount++;
    }
    return q >= 0;
  }

   return true;  // unknown gate, skip
 }
 
 }  // namespace
 
 std::string QirToOriginirTranslator::translate(llvm::Module* module,
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
  out << "QINIT " << totalQubits << "\n";
  for (auto& BB : *Kernel) {
    for (auto& I : BB) {
      CallInst* CI = dyn_cast<CallInst>(&I);
      if (!CI || !CI->getCalledFunction()) continue;
      if (CI->getCalledFunction()->getName() == "__quantum__qis__mz")
        measureCount++;
    }
  }
  if(measureCount > 0) {
    out << "CREG " << measureCount << "\n";
  }
  measureCount = 0;
  // Originir output: one instruction per line.
  // Note: Originir is a control instruction set, not OpenQASM.
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
 
 bool QirToOriginirTranslator::translateToFile(llvm::Module* module,
                                           const std::string& outPath,
                                           const std::string& kernelName) {

   std::string qasm = translate(module, kernelName);
  //  std::cout << qasm << std::endl;
   if (qasm.empty()) return false;
 
   std::ofstream ofs(outPath);
   if (!ofs) return false;
   ofs << qasm;
   ofs.close();
   return true;
 }
 
 std::string qirToOriginir(llvm::Module* module, const std::string& kernelName) {
  
   QirToOriginirTranslator translator;
   return translator.translate(module, kernelName);
 }
 
 bool qirToOriginirFile(llvm::Module* module, const std::string& outPath,
                    const std::string& kernelName) {
   QirToOriginirTranslator translator;
   return translator.translateToFile(module, outPath, kernelName);
 }
 
 }  // namespace qllvm
 