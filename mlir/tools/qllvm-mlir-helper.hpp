/*******************************************************************************
 * Copyright (c) 2018-, UT-Battelle, LLC.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the MIT License 
 * which accompanies this distribution. 
 *
 * Contributors:
 *   Alexander J. McCaskey - initial API and implementation
 *   Thien Nguyen - implementation
 *******************************************************************************/
 #pragma once
 #include <fstream>
 
 #include "Quantum/QuantumDialect.h"
 #include "llvm/Support/TargetSelect.h"
 #include "mlir/Dialect/Affine/IR/AffineOps.h"
 #include "mlir/Dialect/LLVMIR/LLVMDialect.h"
 #include "mlir/Dialect/SCF/SCF.h"
 #include "mlir/Dialect/StandardOps/IR/Ops.h"
 #include "mlir/Dialect/Vector/VectorOps.h"
 #include "mlir/ExecutionEngine/ExecutionEngine.h"
 #include "mlir/ExecutionEngine/OptUtils.h"
 #include "mlir/IR/AsmState.h"
 #include "mlir/Parser.h"
 
 #include "qiskit_mlir_generator.hpp"
 #include "openqasmv3_mlir_generator.hpp"
 #include "qcis_mlir_generator.hpp"
 #include "pass_manager.hpp"
 #include "quantum_to_llvm.hpp"
 // #include "tools/ast_printer.hpp"
 namespace qllvm {
 namespace util {
 // Helper to run common MLIR tasks:
 // enum SourceLanguage { QASM2, QASM3 };
 enum SourceLanguage { QASM3 ,QISKIT,QCIS};
 // Wrapper for MLIR generation results:
 struct MlirGenerationResult {
   std::unique_ptr<mlir::MLIRContext> mlir_context;
   SourceLanguage source_language;
   std::vector<std::string> unique_function_names;
   std::unique_ptr<mlir::OwningModuleRef> module_ref;
   MlirGenerationResult(std::unique_ptr<mlir::MLIRContext> &&in_mlir_context,
                        SourceLanguage in_source_language,
                        const std::vector<std::string> &in_unique_function_names,
                        std::unique_ptr<mlir::OwningModuleRef> &&in_module_ref,
                        bool verbose_error = false)
       : mlir_context(std::move(in_mlir_context)),
         source_language(in_source_language),
         unique_function_names(in_unique_function_names),
         module_ref(std::move(in_module_ref)) {
     // Set the DiagnosticEngine handler
     DiagnosticEngine &engine = (*mlir_context).getDiagEngine();
 
     // Handle the reported diagnostic.
     // Return success to signal that the diagnostic has either been fully
     // processed, or failure if the diagnostic should be propagated to the
     // previous handlers.
     engine.registerHandler(
         [&, verbose_error](Diagnostic &diag) -> LogicalResult {
           if (verbose_error) {
             std::cout << "[qllvm-mlir] Dumping Module after error.\n";
             (*module_ref)->dump();
           }
           std::string BOLD = "\033[1m";
           std::string RED = "\033[91m";
           std::string CLEAR = "\033[0m";
 
           for (auto &n : diag.getNotes()) {
             std::string s;
             llvm::raw_string_ostream os(s);
             n.print(os);
             os.flush();
             std::cout << BOLD << RED << "[qllvm-mlir] Reported Error: " << s << "\n"
                       << CLEAR;
           }
           bool should_propagate_diagnostic = true;
           return failure(should_propagate_diagnostic);
         });
   }
   ~MlirGenerationResult() {}
 };
 
 std::pair<SourceLanguage, std::unique_ptr<mlir::OwningModuleRef>> loadMLIR(
     const std::string &qasm_src, const std::string &kernel_name,
     mlir::MLIRContext &context, std::vector<std::string> &function_names,
     bool addEntryPoint,
     std::map<std::string, std::string> extra_quantum_args = {}) {
 
   SourceLanguage src_language_type = SourceLanguage::QASM3;
   std::string src_to_parse = qasm_src;

   // Route to parser: prefer OpenQasmV3 (stable). OpenQASM 2.0 is converted
   // to 3 preamble to avoid QiskitMLIRGenerator segfault (see issue debug).
   if (qasm_src.find("OPENQASM 3;") != std::string::npos) {
     src_language_type = SourceLanguage::QASM3;
   }  else if (qasm_src.find("OPENQASM 2.0") != std::string::npos) {
    // Preprocess: replace OPENQASM 2.0 with OPENQASM 3 for compatible 2.0 subset
     src_to_parse = qasm_src;
     size_t pos = 0;
     if ((pos = src_to_parse.find("OPENQASM 2.0")) != std::string::npos) {
        src_to_parse.replace(pos, 12, "OPENQASM 3");
      }
      src_language_type = SourceLanguage::QASM3;
   }
   std::string key = "input_backend";
   auto it = extra_quantum_args.find(key);
   if (it != extra_quantum_args.end()) {
     std::string value = it->second;
     if (value == "qcis") {
       src_language_type = SourceLanguage::QCIS;
     }else if (value == "qiskit") {
       src_language_type = SourceLanguage::QISKIT;
     }else {
       std::cout << "No other mlir generators yet.\n";
       exit(1);
     }
   }
 
 
   std::shared_ptr<qllvm::QuantumMLIRGenerator> mlir_generator;
   if (src_language_type == SourceLanguage::QASM3) {
     mlir_generator = std::make_shared<qllvm::OpenQasmV3MLIRGenerator>(context);
   } else if (src_language_type == SourceLanguage::QISKIT) {
     mlir_generator = std::make_shared<qllvm::QiskitMLIRGenerator>(context);
   } else if (src_language_type == SourceLanguage::QCIS) {
     mlir_generator = std::make_shared<qllvm::qcisMLIRGenerator>(context);
   }else {
     std::cout << "No other mlir generators yet.\n";
     exit(1);
   }
 
   mlir_generator->initialize_mlirgen(
       addEntryPoint, kernel_name,
       extra_quantum_args);  // FIXME HANDLE RELATIVE PATH
   mlir_generator->mlirgen(src_to_parse);
   mlir_generator->finalize_mlirgen();
   function_names = mlir_generator->seen_function_names();
   return std::make_pair(src_language_type,
                         std::make_unique<mlir::OwningModuleRef>(
                             std::move(mlir_generator->get_module())));
 }
 
 MlirGenerationResult mlir_gen(
     const std::string &qasm_src, const std::string &kernel_name,
     bool add_entry_point,
     std::map<std::string, std::string> extra_quantum_args = {}) {
   auto context = std::make_unique<mlir::MLIRContext>();
   context->loadDialect<mlir::quantum::QuantumDialect, mlir::AffineDialect,
                        mlir::scf::SCFDialect, mlir::StandardOpsDialect>();
 
   std::vector<std::string> unique_function_names;
   auto [src_type, module] =
       loadMLIR(qasm_src, kernel_name, *context, unique_function_names,
                add_entry_point, extra_quantum_args);
 
   return MlirGenerationResult(std::move(context), src_type,
                               unique_function_names, std::move(module),
                               extra_quantum_args.count("verbose_error"));
 }
 
 MlirGenerationResult mlir_gen(
     const std::string &inputFilename, bool add_entry_point,
     std::string function_name = "",
     std::map<std::string, std::string> extra_quantum_args = {}) {
   llvm::StringRef ref(inputFilename);
   std::ifstream t(ref.str());
   std::string qasm_src((std::istreambuf_iterator<char>(t)),
                        std::istreambuf_iterator<char>());
   if (function_name.empty()) {
     function_name = llvm::sys::path::filename(inputFilename)
                         .split(StringRef("."))
                         .first.str();
   }
   return mlir_gen(qasm_src, function_name, add_entry_point, extra_quantum_args);
 }
 }  // namespace util
 } // namespace qllvm
 