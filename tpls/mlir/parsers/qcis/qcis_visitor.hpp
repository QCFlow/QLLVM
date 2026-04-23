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

#pragma once
#include <regex>
#include <string>

#include "Quantum/QuantumOps.h"
#include "mlir/Dialect/Affine/IR/AffineOps.h"
#include "mlir/Dialect/StandardOps/IR/Ops.h"
#include "mlir/IR/Attributes.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/BuiltinTypes.h"
#include "mlir/IR/MLIRContext.h"
#include "mlir/IR/Verifier.h"
#include "qcisBaseVisitor.h"
#include "qasm3_utils.hpp"
#include "symbol_table.hpp"
#include "expression_handler.hpp"
using namespace qcis;

namespace qllvm {

class qcis_visitor : public qcis::qcisBaseVisitor {
 public:
  // Return the symbol table.
  ScopedSymbolTable* getScopedSymbolTable() { return &symbol_table; }

  // The constructor, instantiates commonly used opaque types
  qcis_visitor(mlir::OpBuilder b, mlir::ModuleOp m, std::string &fname,
                bool enable_nisq_conditional = false)
      : builder(b), file_name(fname), m_module(m),
        enable_nisq_ifelse(enable_nisq_conditional) {
    auto context = b.getContext();
    llvm::StringRef qubit_type_name("Qubit"), array_type_name("Array"),
        result_type_name("Result");
    mlir::Identifier dialect = mlir::Identifier::get("quantum", context);
    qubit_type = mlir::OpaqueType::get(context, dialect, qubit_type_name);
    array_type = mlir::OpaqueType::get(context, dialect, array_type_name);
    result_type = mlir::OpaqueType::get(context, dialect, result_type_name);
    symbol_table.set_op_builder(builder);
  }

  // antlrcpp::Any visitQuantumCircuitDeclaration(qcisParser::QuantumCircuitDeclarationContext *ctx) override;
  antlrcpp::Any visitQuantumGateCall(qcisParser::QuantumGateCallContext *ctx) override;
  antlrcpp::Any visitQuantumMeasurement(qcisParser::QuantumMeasurementContext *ctx) override;


 protected:

  mlir::OpBuilder builder;
  mlir::ModuleOp m_module;
  std::string file_name = "";
  bool enable_nisq_ifelse = false;  

  ScopedSymbolTable symbol_table;

  bool subroutine_return_statment_added = false;
  bool is_return_stmt = false;

  bool export_subroutine_as_callable = false;

  mlir::Type current_function_return_type;

  mlir::Type qubit_type;
  mlir::Type array_type;
  mlir::Type result_type;


  std::vector<mlir::Value> createInstOps_HandleBroadcast(std::string name,
                                     std::vector<mlir::Value> qbit_values,
                                     std::vector<std::string> qbit_names,
                                     std::vector<std::string> symbol_table_qbit_keys,
                                     std::vector<mlir::Value> param_values,
                                     mlir::Location location,
                                     antlr4::ParserRuleContext* context);

  mlir::Value allocate_1d_memory_and_initialize(
      mlir::Location location, int64_t shape, mlir::Type type,
      std::vector<mlir::Value> initial_values,
      llvm::ArrayRef<mlir::Value> initial_indices) {
    if (shape != initial_indices.size()) {
      printErrorMessage(
          "Cannot allocate and initialize memory, shape and number of initial "
          "value indices is incorrect");
    }

    for (const auto &init_val : initial_values) {
      assert(init_val.getType() == type);
    }

    // Allocate
    auto allocation = allocate_1d_memory(location, shape, type);
    // and initialize
    for (int i = 0; i < initial_values.size(); i++) {
      assert(initial_indices[i].getType().isa<mlir::IndexType>());
      builder.create<mlir::StoreOp>(location, initial_values[i], allocation,
                                    initial_indices[i]);
    }
    return allocation;
  }

  mlir::Value allocate_1d_memory(mlir::Location location, int64_t shape,
                                 mlir::Type type) {
    llvm::ArrayRef<int64_t> shaperef(shape);

    auto mem_type = mlir::MemRefType::get(shaperef, type);
    mlir::Value allocation = builder.create<mlir::AllocaOp>(location, mem_type);

    return allocation;
  }

  // Extract qubit index from QubitId token (e.g. "Q0" -> 0, "Q1" -> 1)
  static int parseQubitIdIndex(const std::string& qubitId) {
    if (qubitId.empty() || qubitId[0] != 'Q')
      return -1;
    return std::stoi(qubitId.substr(1));
  }

  template <class NodeType>
  bool hasChildNodeOfType(antlr4::tree::ParseTree &in_node) {
    for (auto &child_node : in_node.children) {
      if (dynamic_cast<NodeType *>(child_node) ||
          hasChildNodeOfType<NodeType>(*child_node)) {
        return true;
      }
    }
    return false;
  }
};

} // namespace qllvm
