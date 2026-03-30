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

#include "qcis_visitor.hpp"
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

antlrcpp::Any qcis_visitor::visitQuantumMeasurement(qcisParser::QuantumMeasurementContext *ctx) {
  auto location = get_location(builder, file_name, ctx);
  auto str_attr = builder.getStringAttr("mz");
  auto measurementargumentlist = ctx->measurementArgumentList();
  auto measurementArgument = measurementargumentlist->QubitId();
  auto qubit_id_str = measurementArgument->getText();
  const int qubit_idx = parseQubitIdIndex(qubit_id_str);
  if (qubit_idx < 0) {
    printErrorMessage("Invalid QubitId format in measurement: " + qubit_id_str,
                      ctx);
    return 0;
  }
  auto value = get_or_extract_qubit("q", static_cast<std::size_t>(qubit_idx),
                                   location, symbol_table, builder);
  auto bit_value = symbol_table.get_symbol("c");
  if (value.getType() == qubit_type) {
    auto instop = builder.create<mlir::quantum::InstOp>(
          location, result_type, str_attr, llvm::makeArrayRef(value),
          llvm::makeArrayRef(std::vector<mlir::Value>{}));
    const std::string qubit_var_name =
          symbol_table.get_symbol_var_name(value);
    if (!qubit_var_name.empty() && qubit_var_name != "q") {
      symbol_table.erase_symbol(qubit_var_name);
    }
    if (bit_value) {
      mlir::Value v = get_or_create_constant_index_value(
          static_cast<std::size_t>(qubit_idx), location, 64, symbol_table,
          builder);
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
} // namespace qllvm