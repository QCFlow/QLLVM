/*******************************************************************************
 * qllvm-compile: QASM -> MLIR -> LLVM IR -> backend
 * Quantum program compiler. Delegates to CompilationPipeline for compilation logic.
 *******************************************************************************/

#include "CompilationPipeline.hpp"
#include "qllvm/passes/SabreOptions.hpp"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/TargetSelect.h"
#include "mlir/IR/AsmState.h"
#include "mlir/IR/MLIRContext.h"
#include "mlir/Pass/PassManager.h"

#include <iostream>
#include <sstream>

namespace cl = llvm::cl;

static cl::opt<std::string> inputFilename(
    cl::Positional, cl::desc("<input openqasm file>"), cl::init("-"),
    cl::value_desc("filename"));

static cl::opt<std::string> qpu("qpu", cl::desc("The quantum coprocessor to compile to."));
static cl::opt<std::string> qrt("qrt", cl::desc("The quantum execution mode: ftqc or nisq."));
static cl::opt<std::string> shots("shots", cl::desc("The number of shots for nisq mode execution."));
static cl::opt<std::string> input_backend("input-backend", cl::desc("The input backend."));
#ifndef ZHUYU
static cl::opt<std::string> placement(
    "placement", cl::desc("The placement strategy: default or sabre-swap."));
#endif
static cl::opt<bool> noEntryPoint("no-entrypoint",
                                  cl::desc("Do not add main() to compiled output."));
static cl::opt<bool> mlir_quantum_opt("q-optimize",
                                      cl::desc("Turn on MLIR-level quantum instruction optimizations."));
static cl::opt<std::string> mlir_specified_func_name(
    "internal-func-name", cl::desc("qllvm provided function name"));
static cl::opt<bool> verbose_error("verbose-error",
                                   cl::desc("Printout the full MLIR Module on error."));
static cl::opt<bool> print_final_submission(
    "print-final-submission", cl::desc("Print the final quantum program representation for backend submission."));
static cl::opt<bool> OptLevelO0("O0", cl::desc("Optimization level 0. Similar to clang -O0."));
static cl::opt<bool> OptLevelO1("O1", cl::desc("Optimization level 1. Similar to clang -O1."));
static cl::opt<bool> EmitMLIR("emitmlir", cl::desc("Print MLIR"));
static cl::opt<bool> EmitQIR("emitqir", cl::desc("Print QIR"));
static cl::opt<bool> EmitQASM(
    "emit-qasm", cl::desc("Emit OpenQASM from QIR (alias for -emit-backend=qasm-backend)"));
static cl::opt<std::string> EmitBackend(
    "emit-backend", cl::desc("Emit code via named backend (e.g. qasm-backend)"), cl::value_desc("name"));
static cl::opt<std::string> OutputQASM(
    "output-qasm", cl::desc("Output file path for -emit-qasm (deprecated, use -output-path)"),
    cl::value_desc("file"));
static cl::opt<std::string> OutputPath("output-path",
                                       cl::desc("Output file path for -emit-backend"),
                                       cl::value_desc("file"));
static cl::opt<std::string> OutputLl("output-ll",
                                     cl::desc("Output path for LLVM IR (.ll) when not using -emit-backend (for hybrid linking)"),
                                     cl::value_desc("file"));
static cl::opt<bool> circuit_state("circuit-state",
                                   cl::desc("Print the circuit state (depth, gate count)."));
static cl::opt<bool> pass_count("pass-count", cl::desc("Print the count of every pass."));
static cl::opt<bool> syn_opt("syn-opt", cl::desc("The switch of synthesis_optimization."));
static cl::opt<bool> pass_effect("pass-effect", cl::desc("The optimization effect of pass."));
static cl::opt<bool> random_seq("random_seq", cl::desc("Use random pass sequences to compile."));
static cl::opt<std::string> customPassSequence(
    "customPassSequence", cl::desc("This file stores pass sequence."), cl::value_desc("file name"));
static cl::opt<std::string> basicgate("basicgate", cl::desc("The set of basicgate."),
                                     cl::value_desc("basicgate"));
static cl::opt<bool> mlir_debug_dialect_conversion(
    "debug-dialect-conversion",
    cl::desc("Debug the execution of the dialect conversion framework."));
static cl::opt<std::string> sabre_coupling_map(
    "sabre-coupling-map",
    cl::desc("SABRE coupling map: edges as '0,1;1,2;2,3' (linear chain). Requires -emit-backend=qasm-backend."),
    cl::value_desc("edges"));

int main(int argc, char** argv) {
  mlir::registerAsmPrinterCLOptions();
  mlir::registerMLIRContextCLOptions();
  mlir::registerPassManagerCLOptions();

  cl::ParseCommandLineOptions(argc, argv, "qllvm quantum assembly compiler\n");

  if (mlir_debug_dialect_conversion) {
    llvm::DebugFlag = true;
    llvm::setCurrentDebugType("dialect-conversion");
  }

  qllvm::CompilationOptions opts;
  opts.inputFile = inputFilename.getValue();
  opts.addEntryPoint = !noEntryPoint;
  opts.kernelName = mlir_specified_func_name.getValue();
  opts.optLevel = OptLevelO1 ? 1 : 0;
  opts.qOptimizations = mlir_quantum_opt || OptLevelO1;
  opts.emitMLIR = EmitMLIR;
  opts.emitQIR = EmitQIR;
  opts.emitBackend = EmitBackend.getValue();
  if (EmitQASM) opts.emitBackend = "qasm-backend";
  opts.outputPath = OutputPath.getValue();
  if (opts.outputPath.empty()) opts.outputPath = OutputQASM.getValue();
  opts.outputLlPath = OutputLl.getValue();
  opts.circuitState = circuit_state;
  opts.passCount = pass_count;
  opts.passEffect = pass_effect;
  opts.synOpt = syn_opt;
  opts.randomSeq = random_seq;
  if (!input_backend.getValue().empty()) opts.extraArgs["input_backend"] = input_backend.getValue();
  if (!customPassSequence.empty()) opts.customPassSequenceFile = customPassSequence.getValue();
  if (!qpu.getValue().empty()) opts.extraArgs["qpu"] = qpu.getValue();
  
  if (!basicgate.getValue().empty()) {
    std::unordered_set<std::string> gates;
    std::string bg = basicgate.getValue();
    std::string content = bg.substr(1, bg.size() - 2);
    std::stringstream ss(content);
    std::string token;
    while (std::getline(ss, token, ',')) gates.insert(token);
    opts.basicGateSet = gates;
  }
  if (opts.extraArgs["qpu"] == "tianyan"){
    std::unordered_set<std::string> gates = {"rx","ry","rz","h","cz"};
    opts.basicGateSet = gates;
  }
  if (opts.extraArgs["qpu"] == "originquantum"){
    std::unordered_set<std::string> gates = {"rx","ry","rz","h","cz"};
    opts.basicGateSet = gates;
  }

  if (!sabre_coupling_map.getValue().empty()) {
    qllvm::sabre::SabreOptions sabreOpts;
    std::string s = sabre_coupling_map.getValue();
    for (size_t i = 0; i < s.size(); ) {
      size_t sep = s.find(';', i);
      std::string pair = (sep == std::string::npos) ? s.substr(i) : s.substr(i, sep - i);
      size_t c = pair.find(',');
      if (c != std::string::npos) {
        int a = std::stoi(pair.substr(0, c));
        int b = std::stoi(pair.substr(c + 1));
        sabreOpts.couplingEdges.push_back({a, b});
      }
      i = (sep == std::string::npos) ? s.size() : sep + 1;
    }
    sabreOpts.layoutTrials = 2;
    sabreOpts.swapTrials = 1;
    sabreOpts.seed = 42;
    opts.sabreOptions = sabreOpts;
  }

  

  if (!qrt.getValue().empty()) opts.extraArgs["qrt"] = qrt.getValue();
  if (!shots.getValue().empty()) opts.extraArgs["shots"] = shots.getValue();
#ifndef ZHUYU
  if (!placement.getValue().empty()) opts.extraArgs["placement"] = placement.getValue();
#endif
  if (verbose_error) opts.extraArgs["verbose_error"] = "";
  if (print_final_submission) opts.extraArgs["print_final_submission"] = "";
  
  qllvm::CompilationResult result = qllvm::runCompilationPipeline(opts);

  if (result.exitCode != 0 && !result.errorMessage.empty()) {
    std::cerr << "\033[1m\033[91m" << result.errorMessage << "\033[0m\n";
  }

  return result.exitCode;
}
