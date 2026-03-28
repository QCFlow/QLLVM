/*******************************************************************************
 * CompilationPipeline implementation
 *******************************************************************************/

#include "CompilationPipeline.hpp"

#include "Quantum/QuantumDialect.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/TargetSelect.h"
#include "mlir/Dialect/Affine/IR/AffineOps.h"
#include "mlir/Dialect/LLVMIR/LLVMDialect.h"
#include "mlir/Dialect/SCF/SCF.h"
#include "mlir/Dialect/StandardOps/IR/Ops.h"
#include "mlir/Dialect/Vector/VectorOps.h"
#include "mlir/ExecutionEngine/OptUtils.h"
#include "mlir/IR/AsmState.h"
#include "mlir/Parser.h"
#include "mlir/Pass/PassManager.h"
#include "mlir/Target/LLVMIR.h"
#include "openqasmv3_mlir_generator.hpp"
#include "pass_manager.hpp"
#include "qllvm-mlir-helper.hpp"
#include "qllvm/BackendOptions.hpp"
#include "qllvm/BackendRegistry.hpp"
#include "qllvm/passes/DecomposeSwapQirPass.hpp"
#include "qllvm/passes/SabrePass.hpp"
#include "qllvm/passes/u3_to_rphi.hpp"
#include "quantum_to_llvm.hpp"

#include <fstream>
#include <iostream>
#include <random>
#include <sstream>

namespace qllvm {

namespace {

constexpr const char* BOLD = "\033[1m";
constexpr const char* RED = "\033[91m";
constexpr const char* CLEAR = "\033[0m";

std::string stemFromPath(const std::string& path) {
  llvm::StringRef ref(path);
  llvm::StringRef filename = llvm::sys::path::filename(ref);
  return filename.split(llvm::StringRef(".")).first.str();
}

/// Build bool_args map from CompilationOptions for pass_manager
std::map<std::string, bool> buildBoolArgs(const CompilationOptions& opts) {
  std::map<std::string, bool> m;
  if (opts.circuitState) m["circuit_state"] = true;
  if (opts.passCount) m["pass_count"] = true;
  if (opts.passEffect) m["pass_effect"] = true;
  if (opts.synOpt) m["syn_opt"] = true;
  if (opts.randomSeq) m["random_seq"] = true;
  if (opts.customPassSequenceFile) m["customPassSequence"] = true;
  if (opts.basicGateSet) m["basicgate"] = true;
  return m;
}

/// Generate pass sequence for syn_opt/random_seq (used by configureOptimizationPasses)
std::vector<int> getPassSequence(const std::map<std::string, bool>& boolArgs) {
  std::random_device rd;
  std::default_random_engine eng(rd());
  std::uniform_int_distribution<int> pass(3, 6);
  std::uniform_int_distribution<int> Random(0, 8);
  std::vector<int> seq;
  const std::vector<int> after = {REMOVE_DIAGONAL_GATES_BEFORE_MEASURE,
                                  REMOVE_UNUSED_QIR_CALLS_PASS};

  if (boolArgs.count("random_seq")) {
    for (int n = 0; n < 30; n++) seq.push_back(Random(eng));
    seq.push_back(8);
    return seq;
  }
  for (int k = 0; k < 2; k++) {
    for (int n = 0; n < 240; n++) seq.push_back(pass(eng));
  }
  for (size_t n = 0; n < after.size(); n++) seq.push_back(after[n]);
  return seq;
}

void printPassCountStats(int passes_count[20]) {
  std::cout << "======================================" << std::endl;
  int sum = 0;
  for (int p = 0; p < 20; p++) {
    sum += passes_count[p];
    if (passes_count[p] == 0) continue;
    switch (p) {
      case CONSOLIDATEBLOCKS:
        std::cout << "CONSOLIDATEBLOCKS run count: " << passes_count[p] << std::endl;
        break;
      case COMMUTATIVECANCELLATIONPASS:
        std::cout << "COMMUTATIVECANCELLATIONPASS run count: " << passes_count[p] << std::endl;
        break;
      case REMOVE_DIAGONAL_GATES_BEFORE_MEASURE:
        std::cout << "REMOVE_DIAGONAL_GATES_BEFORE_MEASURE run count: " << passes_count[p] << std::endl;
        break;
      case SINGLE_QUBIT_IDENTITY_PAIR_REMOVALPASS:
        std::cout << "SINGLE_QUBIT_IDENTITY_PAIR_REMOVALPASS run count: " << passes_count[p] << std::endl;
        break;
      case CNOTIDENTITYPAIRREMOVALPASS:
        std::cout << "CNOTIDENTITYPAIRREMOVALPASS run count: " << passes_count[p] << std::endl;
        break;
      case DUPLICATERESETREMOVALPASS:
        std::cout << "DUPLICATERESETREMOVALPASS run count: " << passes_count[p] << std::endl;
        break;
      case SINGLE_QUBIT_GATE_MERGING_PASS:
        std::cout << "SINGLE_QUBIT_GATE_MERGING_PASS run count: " << passes_count[p] << std::endl;
        break;
      case REMOVE_UNUSED_QIR_CALLS_PASS:
        std::cout << "REMOVE_UNUSED_QIR_CALLS_PASS run count: " << passes_count[p] << std::endl;
        break;
      case CIRUIT_STATE1:
        std::cout << "CIRUIT_STATE run count: 2" << std::endl;
        break;
      default:
        break;
    }
  }
  std::cout << "Total: " << sum << std::endl;
}

}  // namespace

CompilationResult runCompilationPipeline(const CompilationOptions& opts) {
  CompilationResult result;

  auto mlir_gen_result = qllvm::util::mlir_gen(
      opts.inputFile, opts.addEntryPoint, opts.kernelName, opts.extraArgs);

  mlir::OwningModuleRef& module = *(mlir_gen_result.module_ref);
  mlir::MLIRContext& context = *(mlir_gen_result.mlir_context);
  std::vector<std::string>& unique_function_names = mlir_gen_result.unique_function_names;

  std::string fname = stemFromPath(opts.inputFile);

  // module->dump();
  // Delete unused MLIR functions
  mlir::PassManager pm_delete(&context);
  qllvm::configureRemoveUnusedPasses(pm_delete, opts.inputFile);
  if (mlir::failed(pm_delete.run((*module).getOperation()))) {
    result.exitCode = 1;
    result.errorMessage = "[qllvm-compile] Delete unused IR failed.";
    return result;
  }

  // Optimization passes
  mlir::PassManager pm(&context);
  mlir::applyPassManagerCLOptions(pm);

  qllvm::PassManagerOptions pmOpts;
  pmOpts.inputFile = fname;
  pmOpts.boolArgs = buildBoolArgs(opts);
  if (opts.basicGateSet) pmOpts.basicGateSet = *opts.basicGateSet;

  qllvm::PassManagerStats pmStats;

  if (opts.circuitState) {
    pm.addPass(std::make_unique<qllvm::circuitState>(pmOpts.boolArgs, pmStats.passes_count[CIRUIT_STATE1]));
  }
  std::unordered_set<std::string> temp_set = {"u3","cz"};
  bool turn_to_u3 = false;
  if (pmOpts.basicGateSet  == temp_set){
    turn_to_u3 = true;
    pmOpts.basicGateSet = {"rx","ry","rz","h","cz"};
  }
  
  if (opts.emitBackend == "originquantum"){
    turn_to_u3 = true;
  }
  
  pm.addPass(std::make_unique<qllvm::DecomposemultiPass>());
  if (opts.optLevel == 0){
    pm.addPass(std::make_unique<qllvm::Check_gate>());
  }

  if (opts.optLevel >= 1) {
    if (opts.customPassSequenceFile) {
      std::ifstream infile(*opts.customPassSequenceFile);
      if (!infile.is_open()) {
        result.exitCode = 1;
        result.errorMessage = "Failed to open custom pass sequence file.";
        return result;
      }
      std::string buf;
      while (std::getline(infile, buf)) pmOpts.customPassSequence.push_back(std::atoi(buf.c_str()));
      qllvm::configureOptimizationPasses(pm, pmOpts, &pmStats);
    } else if (opts.synOpt || opts.randomSeq) {
      pmOpts.customPassSequence = getPassSequence(pmOpts.boolArgs);
      qllvm::configureOptimizationPasses(pm, pmOpts, &pmStats);
    } else if (!pmOpts.basicGateSet.empty()) {
      qllvm::configureOptimizationPasses(pm, pmOpts, nullptr);
    } else {
      qllvm::configureOptimizationPasses(pm, pmOpts, nullptr);
    }
  }

  if (!pmOpts.basicGateSet.empty() && opts.optLevel == 0) {
    pm.addPass(std::make_unique<qllvm::trans_basicgate>(pmOpts.basicGateSet));
  }
  
  if (turn_to_u3){
    pm.addPass(std::make_unique<qllvm::Merge_u3_gate>());
  }


  if (opts.circuitState) {
    pm.addPass(std::make_unique<qllvm::circuitState2>(pmOpts.boolArgs, pmStats.passes_count[CIRUIT_STATE2]));
  }

  if (mlir::failed(pm.run((*module).getOperation()))) {
    result.exitCode = 1;
    result.errorMessage = "[qllvm-compile] MLIR optimization failed.";
    return result;
  }
  

  if (opts.emitMLIR) {
    std::cout << "=================MLIR======================" << std::endl;
    std::string mlir_str;
    llvm::raw_string_ostream os(mlir_str);
    (*module).getOperation()->print(os);
    os.flush();
    std::cout << mlir_str << std::endl;
  }

  // Lower to LLVM MLIR
  mlir::PassManager pm_llvm(&context);
  pm_llvm.addPass(std::make_unique<qllvm::ModifierRegionRewritePass>());
  pm_llvm.addPass(std::make_unique<qllvm::QuantumToLLVMLoweringPass>(
      opts.qOptimizations, unique_function_names));
  if (mlir::failed(pm_llvm.run((*module).getOperation()))) {
    result.exitCode = 1;
    result.errorMessage = "[qllvm-compile] MLIR-to-LLVM lowering failed.";
    return result;
  }

  if (opts.passCount) printPassCountStats(pmStats.passes_count);

  if (opts.passEffect) {
    std::cout << "======================================" << std::endl;
    const int opt_pass[] = {CONSOLIDATEBLOCKS, COMMUTATIVECANCELLATIONPASS,
                           SINGLE_QUBIT_IDENTITY_PAIR_REMOVALPASS, CNOTIDENTITYPAIRREMOVALPASS,
                           DUPLICATERESETREMOVALPASS, SINGLE_QUBIT_GATE_MERGING_PASS,
                           REMOVE_DIAGONAL_GATES_BEFORE_MEASURE};
    const std::vector<std::string> names = {
        "CONSOLIDATEBLOCKS", "COMMUTATIVECANCELLATIONPASS",
        "SINGLE_QUBIT_IDENTITY_PAIR_REMOVALPASS", "CNOTIDENTITYPAIRREMOVALPASS",
        "DUPLICATERESETREMOVALPASS", "SINGLE_QUBIT_GATE_MERGING_PASS",
        "REMOVE_DIAGONAL_GATES_BEFORE_MEASURE"};
    for (size_t n = 0; n < sizeof(opt_pass) / sizeof(opt_pass[0]); n++) {
      std::cout << names[n] << ": gate count " << pmStats.opt_count[opt_pass[n]]
                << " depth " << pmStats.opt_depth[opt_pass[n]] << std::endl;
    }
  }

  // MLIR -> LLVM IR
  llvm::LLVMContext llvmContext;
  auto llvmModule = mlir::translateModuleToLLVMIR(*module, llvmContext);
  if (!llvmModule) {
    result.exitCode = -1;
    result.errorMessage = "[qllvm-compile] MLIR-to-LLVM-IR lowering failed.";
    return result;
  }

  llvm::InitializeNativeTarget();
  llvm::InitializeNativeTargetAsmPrinter();
  unsigned optLevel = (opts.optLevel >= 1) ? 1 : 0;
  // Do not run LLVM IR optimization before SABRE: InstCombine/others can
  // reorder __quantum__qis__* calls, which breaks the circuit order that
  // CircuitExtractor/Sabre assume and makes mapped output non-equivalent.
  const bool sabreEnabled =
      opts.sabreOptions.has_value() && opts.sabreOptions->enabled();
  if (!sabreEnabled) {
    auto optPipeline = mlir::makeOptimizingTransformer(optLevel, 0, nullptr);
    if (auto err = optPipeline(llvmModule.get())) {
      result.exitCode = -1;
      result.errorMessage = "Failed to optimize LLVM IR";
      return result;
    }
  }

  // SABRE mapping (Strategy B: modify LLVM IR before QirToQasm)
  if (opts.sabreOptions.has_value() && opts.sabreOptions->enabled()) {
    std::string kernelName = opts.kernelName.empty() ? fname : opts.kernelName;
    qllvm::sabre::runSabre(llvmModule.get(), *opts.sabreOptions, kernelName);
    // std::cout << "================================================" << std::endl;
    // llvmModule->dump();
    
    // Decompose swap by basicGateSet (cx/cnot -> 3×CNOT; cz+h -> CZ+H; swap -> keep)
    std::unordered_set<std::string> gateSet;
    if (opts.basicGateSet) gateSet = *opts.basicGateSet;
    if (opts.emitBackend == "originquantum") gateSet = {"u3","cz"};
    qllvm::sabre::runDecomposeSwapQir(llvmModule.get(), gateSet);
  }

  if (opts.emitBackend == "originquantum"){
    qllvm::sabre::u3_to_rphi(llvmModule.get());
  }

  if (opts.emitQIR) {
    std::cout << "=================QIR======================" << std::endl;
    std::string irString;
    llvm::raw_string_ostream rso(irString);
    llvmModule->print(rso, nullptr);
    rso.flush();
    std::cout << irString << std::endl;
  }
  

  // Emit via backend
  if (!opts.emitBackend.empty()) {
    // std::cout << "=================EmitBackend======================" << std::endl;
    auto* backend = BackendRegistry::instance().get(opts.emitBackend);
    if (!backend) {
      result.exitCode = 1;
      result.errorMessage = "[qllvm-compile] Unknown backend: " + opts.emitBackend;
      return result;
    }
    std::string outPath = opts.outputPath.empty() ? "compiled.qasm" : opts.outputPath;
    std::string kernelName = opts.kernelName.empty() ? fname : opts.kernelName;
    BackendOptions beOpts;

    beOpts.outputPath = outPath;
    beOpts.kernelName = kernelName;
    beOpts.format = "openqasm2";
    if (!backend->emit(llvmModule.get(), beOpts)) {
      result.exitCode = 1;
      result.errorMessage = "[qllvm-compile] Backend " + opts.emitBackend + " emit failed.";
      return result;
    }
    return result;
  }

  // Write .ll file
  std::string llPath = opts.outputLlPath.empty() ? fname + ".ll" : opts.outputLlPath;
  std::string s;
  llvm::raw_string_ostream os(s);
  llvmModule->print(os, nullptr, false, true);
  os.flush();
  std::ofstream outFile(llPath);
  if (!outFile) {
    result.exitCode = 1;
    result.errorMessage = "Failed to write " + llPath;
    return result;
  }
  outFile << s;
  result.emittedLlPath = llPath;
  return result;
}

}  // namespace qllvm
