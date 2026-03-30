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

#include "qiskit_visitor.hpp"

namespace qllvm {

antlrcpp::Any qiskit_visitor::visitQuantumCircuitDeclaration(
    qiskitParser::QuantumCircuitDeclarationContext* context) {
  // quantumCircuitDeclaration
  // : circuitName '=' 'QuantumCircuit' '(' quantumCount (',' classicalCount)? ')' SEMICOLON
  // ;
  //   quantumCount
  //     : Integer
  //     ;

  // classicalCount
  //     : Integer
  //     ;
  
  // can be
  // cir = QuantumCircuit(a);
  // cir = QuantumCircuit(a,b);

  auto location = get_location(builder, file_name, context);

  auto quantumcount = context->quantumCount()->getText();
  int64_t size = std::stoi(quantumcount);

  std::string var_name = "q";
  auto integer_type = builder.getI64Type();

  auto integer_attr = mlir::IntegerAttr::get(integer_type, size);

  auto str_attr = builder.getStringAttr(var_name);
  mlir::Value allocation = builder.create<mlir::quantum::QallocOp>(
      location, array_type, integer_attr, str_attr);
  symbol_table.add_symbol(var_name, allocation);
  
  if(context->classicalCount()){
    std::string c_name = "c";
    auto classicalcount = context->classicalCount()->getText();
    size = std::stoi(classicalcount);

    std::vector<mlir::Value> init_values, init_indices;
    for (std::size_t i = 0; i < size; i++) {
      init_values.push_back(get_or_create_constant_integer_value(
          0, location, builder.getI1Type(), symbol_table, builder));
      init_indices.push_back(get_or_create_constant_index_value(
          i, location, 64, symbol_table, builder));
    }
    if (size == 1) {
      llvm::ArrayRef<int64_t> shaperef{};
      auto mem_type = mlir::MemRefType::get(shaperef, builder.getI1Type());
      mlir::Value allocation =
          builder.create<mlir::AllocaOp>(location, mem_type);

      // Store the value to the 0th index of this storeop
      builder.create<mlir::StoreOp>(location, init_values[0], allocation);
      symbol_table.add_symbol(c_name, allocation);
    } else {
      auto allocation = allocate_1d_memory_and_initialize(
          location, size, builder.getI1Type(), init_values,
          llvm::makeArrayRef(init_indices));
      symbol_table.add_symbol(c_name, allocation);
    }
  }

  return 0;

}

}  // namespace qllvm
