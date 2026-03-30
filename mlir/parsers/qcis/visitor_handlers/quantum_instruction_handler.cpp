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

std::vector<mlir::Value> qcis_visitor::createInstOps_HandleBroadcast(
    std::string name, std::vector<mlir::Value> qbit_values,
    std::vector<std::string> qreg_names,
    std::vector<std::string> symbol_table_qbit_keys,
    std::vector<mlir::Value> param_values, mlir::Location location,
    antlr4::ParserRuleContext* context) {
  std::vector<mlir::Value> updated_qubit_values;
  
  auto has_array_type = [this](auto& value_vector) {
    for (auto v : value_vector) {
      if (v.getType() == array_type) {
        return true;
      }
    }
    return false;
  };

  auto get_qreg_size = [&, this](mlir::Value qreg_value,
                                 const std::string qreg_name) {
    uint64_t nqubits = 0;
    if (auto op = qreg_value.getDefiningOp<mlir::quantum::QallocOp>()) {
      nqubits = op.size().getLimitedValue();
    } else {
      auto attributes = symbol_table.get_variable_attributes(qreg_name);
      if (!attributes.empty()) {
        try {
          nqubits = std::stoi(attributes[0]);
        } catch (...) {
          printErrorMessage("Could not infer qubit[] size from block argument.",
                            context);
        }
      } else {
        printErrorMessage(
            "Could not infer qubit[] size from block argument. No size "
            "attribute for variable in symbol table.",
            context);
      }
    }
    return nqubits;
  };
  
  auto str_attr = builder.getStringAttr(name);
  // FIXME Extremely hacky way to handle gate broadcasting
  // The cases we consider are...
  // QINST qubit
  // QINST qubit[] -> QINST qubit[j] for all j
  // QINST qubit qubit
  // QINST qubit qubit[] -> QINST qubit qubit[j] for all j
  // QINST qubit[] qubit -> QINST qubit[j] qubit for all j
  // QINST qubit[] qubit[] -> QINST qubit[j] qubit[j] for all j
  if (has_array_type(qbit_values)) {
    if (qbit_values.size() == 1) {
      auto n = get_qreg_size(qbit_values[0], qreg_names[0]);
      for (int i = 0; i < n; i++) {
        auto qubit_type = get_custom_opaque_type("Qubit", builder.getContext());

        auto extract_value = get_or_extract_qubit(qreg_names[0], i, location,
                                                  symbol_table, builder);

        std::vector<mlir::Type> ret_types;
        for (auto q : qbit_values) {
          ret_types.push_back(qubit_type);
        }
        auto inst = builder.create<mlir::quantum::ValueSemanticsInstOp>(
            location, llvm::makeArrayRef(ret_types), str_attr,
            llvm::makeArrayRef(extract_value),
            llvm::makeArrayRef(param_values));

        // Replace qbit_values in symbol table with new result qubits
        auto return_vals = inst.result();
        int ii = 0;
        for (auto result : return_vals) {
          symbol_table.replace_symbol(extract_value, result);
          updated_qubit_values.emplace_back(result);
          ii++;
        }
      }
    } else if (qbit_values.size() == 2) {
      if (qbit_values[0].getType() == array_type &&
          qbit_values[1].getType() == array_type) {
        auto n = get_qreg_size(qbit_values[0], qreg_names[0]);
        auto m = get_qreg_size(qbit_values[1], qreg_names[1]);

        // This case is cx qarray, rarray;

        if (n != m) {
          printErrorMessage("Gate broadcast must be on registers of same size.",
                            context);
        }

        for (int i = 0; i < n; i++) {
          auto qubit_type =
              get_custom_opaque_type("Qubit", builder.getContext());

          auto extract_value_n = get_or_extract_qubit(
              qreg_names[0], i, location, symbol_table, builder);
          auto extract_value_m = get_or_extract_qubit(
              qreg_names[1], i, location, symbol_table, builder);

          std::vector<mlir::Type> ret_types;
          for (auto q : qbit_values) {
            ret_types.push_back(qubit_type);
          }
          auto inst = builder.create<mlir::quantum::ValueSemanticsInstOp>(
              location, llvm::makeArrayRef(ret_types), str_attr,
              llvm::makeArrayRef({extract_value_n, extract_value_m}),
              llvm::makeArrayRef(param_values));

          // Replace qbit_values in symbol table with new result qubits
          auto return_vals = inst.result();
          int ii = 0;
          for (auto result : return_vals) {
            symbol_table.replace_symbol(
                (ii == 0 ? extract_value_n : extract_value_m), result);
            updated_qubit_values.emplace_back(result);
            ii++;
          }
        }

      } else if (qbit_values[0].getType() == array_type &&
                 qbit_values[1].getType() != array_type) {
        auto n = get_qreg_size(qbit_values[0], qreg_names[0]);
        mlir::Value v = qbit_values[1];

        for (int i = 0; i < n; i++) {
          auto qubit_type =
              get_custom_opaque_type("Qubit", builder.getContext());

          // This case is cx qarray, r;

          auto extract_value = get_or_extract_qubit(qreg_names[0], i, location,
                                                    symbol_table, builder);

          std::vector<mlir::Type> ret_types;
          for (auto q : qbit_values) {
            ret_types.push_back(qubit_type);
          }
          auto inst = builder.create<mlir::quantum::ValueSemanticsInstOp>(
              location, llvm::makeArrayRef(ret_types), str_attr,
              llvm::makeArrayRef({extract_value, v}),
              llvm::makeArrayRef(param_values));

          // Replace qbit_values in symbol table with new result qubits
          auto return_vals = inst.result();
          int ii = 0;
          for (auto result : return_vals) {
            symbol_table.replace_symbol((ii == 0 ? extract_value : v), result);
            updated_qubit_values.emplace_back(result);
            ii++;
          }
          v = return_vals[1];
        }
      } else if (qbit_values[0].getType() != array_type &&
                 qbit_values[1].getType() == array_type) {
        auto n = get_qreg_size(qbit_values[1], qreg_names[1]);
        // This is cx q, rarray

        mlir::Value v = qbit_values[0];
        for (int i = 0; i < n; i++) {
          auto qubit_type =
              get_custom_opaque_type("Qubit", builder.getContext());

          auto extract_value = get_or_extract_qubit(qreg_names[1], i, location,
                                                    symbol_table, builder);

          std::vector<mlir::Type> ret_types;
          for (auto q : qbit_values) {
            ret_types.push_back(qubit_type);
          }
          auto inst = builder.create<mlir::quantum::ValueSemanticsInstOp>(
              location, llvm::makeArrayRef(ret_types), str_attr,
              llvm::makeArrayRef({v, extract_value}),
              llvm::makeArrayRef(param_values));

          // Replace qbit_values in symbol table with new result qubits
          auto return_vals = inst.result();
          int ii = 0;
          for (auto result : return_vals) {
            symbol_table.replace_symbol((ii == 0 ? v : extract_value), result);
            updated_qubit_values.emplace_back(result);
            ii++;
          }
          v = return_vals[0];
        }
      }
    } else {
      printErrorMessage(
          "can only broadcast gates with one or two qubit registers");
    }
  } else {
    if (symbol_table_qbit_keys.empty()) {
      builder.create<mlir::quantum::InstOp>(
          location, mlir::NoneType::get(builder.getContext()), str_attr,
          llvm::makeArrayRef(qbit_values), llvm::makeArrayRef(param_values));
    } else {
      std::vector<mlir::Type> ret_types;
      for (auto q : qbit_values) {
        ret_types.push_back(qubit_type);
      }

      auto inst = builder.create<mlir::quantum::ValueSemanticsInstOp>(
          location, llvm::makeArrayRef(ret_types), str_attr,
          llvm::makeArrayRef(qbit_values), llvm::makeArrayRef(param_values));

      // Replace qbit_values in symbol table with new result qubits
      auto return_vals = inst.result();
      int i = 0;
      for (auto result : return_vals) {
        symbol_table.replace_symbol(qbit_values[i], result);
        updated_qubit_values.emplace_back(result);
        i++;
      }
    }
  }
  return updated_qubit_values;
}

void get_param_values(std::vector<qcisParser::ExpressionContext *> expression_list,std::vector<mlir::Value> &param_values,
  mlir::OpBuilder &builder,ScopedSymbolTable &symbol_table,std::string file_name,
  qcisParser::QuantumGateCallContext* context,mlir::Location location){
  for (auto expression : expression_list) {
  // add parameter values:
    qcis_expression_generator qubit_exp_generator(builder, symbol_table,
                                            file_name);
    qubit_exp_generator.visit(expression);
    auto variable_value = qubit_exp_generator.current_value;
    if (variable_value.getType().isa<mlir::MemRefType>()) {
      if (!variable_value.getType()
          .cast<mlir::MemRefType>()
          .getElementType()
          .isIntOrFloat()) {
          printErrorMessage(
            "Variable classical parameter for quantum instruction is not a "
            "float or int.",
            context, {variable_value});
      }

      auto shape =
      variable_value.getType().cast<mlir::MemRefType>().getShape();
      if (!shape.empty() && shape[0] == 1) {
        variable_value = builder.create<mlir::LoadOp>(
          location, variable_value,
          get_or_create_constant_index_value(0, location, 64, symbol_table,
                                            builder));
      } else if (shape.empty()) {
        variable_value =
          builder.create<mlir::LoadOp>(location, variable_value);
      }
    }else{
    // int param -> Float param
      if(variable_value.getType().isInteger(64)){
        if(auto const_def_op = variable_value.getDefiningOp<mlir::ConstantOp>()){
          auto param = const_def_op.getValue().cast<mlir::IntegerAttr>().getInt();
          variable_value = builder.create<mlir::ConstantOp>(location,
                  mlir::FloatAttr::get(builder.getF64Type(),param));
        }
      }
    }
    param_values.push_back(variable_value);
  }
}


antlrcpp::Any qcis_visitor::visitQuantumGateCall(qcisParser::QuantumGateCallContext* context) {
  // quantumGateCall: quantumGateName QubitId ( gateArgumentList | QubitId )?
  // gateArgumentList: gateArgument (single: expression or Integer)
  auto location = get_location(builder, file_name, context);

  auto name = context->quantumGateName()->getText();
  
  // First QubitId is always present
  auto qubitIds = context->QubitId();
  std::vector<std::string> qreg_names;
  std::vector<std::string> qubit_symbol_table_keys;
  std::vector<mlir::Value> qbit_values;
  std::vector<mlir::Value> param_values;
  std::unordered_set<std::string> gate_set = {"X2P","X2M","Y2P","Y2M","RZ","XY2P","XY2M","CZ"};
  std::unordered_set<std::string> one_qbit = {"X2P","X2M","Y2P","Y2M"};
  std::unordered_set<std::string> one_qbit_ro = {"RZ","XY2P","XY2M"};
  std::unordered_set<std::string> two_qbit = {"CZ"};
  std::unordered_set<std::string> three_qbit = {"CCZ"};


  if (gate_set.find(name) != gate_set.end()) {
    for (size_t i = 0; i < qubitIds.size(); i++) {
      auto qubitIdNode = qubitIds[i];
      auto qubitIdStr = qubitIdNode->getText();
      const auto idx_val = qcis_visitor::parseQubitIdIndex(qubitIdStr);
      if (idx_val < 0) {
        printErrorMessage("Invalid QubitId format: " + qubitIdStr, context);
      }
  
      qreg_names.push_back("q");
      auto value = get_or_extract_qubit("q", idx_val, location, symbol_table, builder);
      auto qubit_symbol_name = symbol_table.array_qubit_symbol_name("q", std::to_string(idx_val));
      qbit_values.push_back(value);
      qubit_symbol_table_keys.push_back(qubit_symbol_name);
    }
    auto gateArgList = context->gateArgumentList();
    if (gateArgList) {
      auto gateArg = gateArgList->expression();
      if (gateArg.size() > 0) {
        get_param_values(gateArg, param_values, builder, symbol_table,
          file_name, context, location);
      }
    }
    if(name == "CZ"){
      name = "cz";
    }else if(name == "RZ"){
      name = "rz";
    }

    if (!qbit_values.empty()) {
      createInstOps_HandleBroadcast(name, qbit_values, qreg_names,
                                    qubit_symbol_table_keys, param_values,
                                    location, context);
    }

  }

  // Optional: gateArgumentList (single parameter for gates like RZ, XY2P, XY2M)
  

  return 0;
}


} // namespace qllvm