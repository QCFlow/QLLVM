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
#include <map>
#include <string>
#include <vector>

#include "llvm/ADT/StringRef.h"
#include "mlir/IR/Attributes.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/BuiltinTypes.h"
#include "mlir/IR/MLIRContext.h"
#include "mlir/IR/Verifier.h"

namespace qllvm {
/// Parser base class (MLIR generator) for quantum source languages.
/// OpenQasmV3MLIRGenerator and QiskitMLIRGenerator extend this.
class QuantumMLIRGenerator {
 protected:
  mlir::MLIRContext& context;
  mlir::ModuleOp m_module;
  mlir::OpBuilder builder;
  mlir::Block* main_entry_block;
  std::vector<std::string> function_names;

  // Common parser state (shared by OpenQASM3 and Qiskit parsers)
  std::string file_name = "main";
  bool add_entry_point = true;
  bool add_custom_return = false;
  bool enable_qir_apply_ifelse = false;
  mlir::Type return_type;
  mlir::Type qubit_type;
  mlir::Type array_type;
  mlir::Type result_type;
  std::map<std::string, mlir::Value> global_symbol_table;
  bool add_main = true;

  /// Initialize common opaque types (Qubit, Array, Result) used by parsers.
  void initCommonOpaqueTypes() {
    llvm::StringRef qubit_type_name("Qubit"), array_type_name("Array"),
        result_type_name("Result");
    mlir::Identifier dialect = mlir::Identifier::get("quantum", &context);
    qubit_type = mlir::OpaqueType::get(&context, dialect, qubit_type_name);
    array_type = mlir::OpaqueType::get(&context, dialect, array_type_name);
    result_type = mlir::OpaqueType::get(&context, dialect, result_type_name);
  }

 public:
  QuantumMLIRGenerator(mlir::MLIRContext& ctx) : context(ctx), builder(&ctx) {}
  QuantumMLIRGenerator(mlir::OpBuilder b, mlir::MLIRContext& ctx)
      : context(ctx), builder(b) {}

  // This method can be implemented by subclasses to
  // introduce any initialization steps required for constructing
  // mlir using the quantum dialect. This may also be used for
  // introducing any initialization operations before
  // generation of the rest of the mlir code.
  virtual void initialize_mlirgen(bool add_entry_point = true,
                                  const std::string file_name = "", std::map<std::string, std::string> extra_quantum_args = {}) = 0;

  // This method can be implemented by subclasses to map a
  // quantum code in a subclass-specific source language to
  // the internal generated MLIR ModuleOp instance
  virtual void mlirgen(const std::string& src) = 0;

  // Return the generated ModuleOp
  mlir::OwningModuleRef get_module() {
    return mlir::OwningModuleRef(mlir::OwningOpRef<mlir::ModuleOp>(m_module));
  }
  
  std::vector<std::string> seen_function_names() { return function_names; }

  // Finalize method, override to provide any end operations
  // to the module (like a return_op).
  virtual void finalize_mlirgen() = 0;
};
} // namespace qllvm