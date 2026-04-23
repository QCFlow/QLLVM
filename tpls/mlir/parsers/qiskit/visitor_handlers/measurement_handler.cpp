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

#include "qiskit_visitor.hpp"
#include "expression_handler.hpp"
namespace qllvm {

// quantumMeasurement
//     : circuitName '.' 'measure' '(' measurementArgumentList ')' SEMICOLON
//     ;


// measurementArgumentList
    // : measurementArgument (',' measurementArgument)*
    // ;

// measurementArgument
//     : Integer                    # IntegerMeasurementArgument
//     | qubitReference            # QubitMeasurementArgument
//     ;

antlrcpp::Any qiskit_visitor::visitQuantumMeasurement(qiskitParser::QuantumMeasurementContext *ctx) {
  // std::cout << "visitQuantumMeasurement" << std::endl;
  auto location = get_location(builder, file_name, ctx);
  auto str_attr = builder.getStringAttr("mz");
  auto measurementargumentlist = ctx->measurementArgumentList();
  auto measurementArgument = measurementargumentlist->measurementArgument();
  auto bit_variable = measurementArgument.back()->getText();
  auto idx_str = measurementArgument.front()->getText();
  auto value = symbol_table.get_symbol("q");
  auto bit_value = symbol_table.get_symbol("c");
  // std::cout << "idx_str: " << idx_str << std::endl;
  // std::cout << "bit_variable: " << bit_variable << std::endl;
  value = get_or_extract_qubit("q", std::stoi(idx_str),
                                     location, symbol_table, builder);
  if (value.getType() == qubit_type) {
    auto instop = builder.create<mlir::quantum::InstOp>(
          location, result_type, str_attr, llvm::makeArrayRef(value),
          llvm::makeArrayRef(std::vector<mlir::Value>{}));
    const std::string qubit_var_name =
          symbol_table.get_symbol_var_name(value);
    if (!qubit_var_name.empty() && qubit_var_name != "q") {
      symbol_table.erase_symbol(qubit_var_name);
    }
    mlir::Value v;
    if(measurementArgument.back()){
      size_t idx_str_s = std::stoull(bit_variable);
      v = get_or_create_constant_index_value(
          idx_str_s, location, 64, symbol_table, builder);
      assert(v.getType().isa<mlir::IndexType>());
      auto cast_bit_op = builder.create<mlir::quantum::ResultCastOp>(
            location, builder.getIntegerType(1), instop.bit());
      builder.create<mlir::StoreOp>(
                location, cast_bit_op.bit_result(), bit_value,
                llvm::makeArrayRef(std::vector<mlir::Value>{v}));
    }
    
  }

  
  return 0;
}
}  // namespace qllvm