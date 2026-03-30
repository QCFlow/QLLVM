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

std::vector<mlir::Value> qiskit_visitor::createInstOps_HandleBroadcast(
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

void get_param_values(std::vector<qiskitParser::ExpressionContext *> expression_list,std::vector<mlir::Value> &param_values,
              mlir::OpBuilder &builder,ScopedSymbolTable &symbol_table,std::string file_name,
              qiskitParser::QuantumGateCallContext* context,mlir::Location location){
  for (auto expression : expression_list) {
      // add parameter values:
      qiskit_expression_generator qubit_exp_generator(builder, symbol_table,
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


antlrcpp::Any qiskit_visitor::visitQuantumGateCall(qiskitParser::QuantumGateCallContext* context) {
  // quantumGateCall
  // : circuitName '.' quantumGateName '(' gateArgumentList? ')' SEMICOLON
  // ;

  // quantumGateName
  // : 'h'
  // | 'x'
  // | 'y'
  // | 'z'
  // | 'cx'
  // | 'CX'
  // | 'U'
  // | 'reset'
  // | Identifier
  // ;

  // std::cout << "qiskit_visitor::visitQuantumGateCall" << std::endl;
  auto location = get_location(builder, file_name, context);

  auto name = context->quantumGateName()->getText();
  if(name == "id" || name == "barrier"){
    return 0;
  }
  
  auto gateargumentlist = context->gateArgumentList()->getText();
  auto arg_size =  context->gateArgumentList()->expression().size();

  std::unordered_set<std::string> gate_set = {"h","x","y","z","s","sdg","t","tdg","sx","sxdg",
                                              "ry","rz","rx","p",
                                              "cx","cy","cz","swap","csx","ch","dcx","ecr","cs","iswap","csdg",
                                              "crx","cry","crz","cp","rzz","rxx","rzx","ryy",
                                              "ccx","cswap","rccx","ccz",
                                              "rcccx",
                                              "u",
                                              "cu",
                                              "r",
                                              "mcrx","mcry","mcrz","mcp","mcx","ms"};

  std::unordered_set<std::string> one_qbit = {"h","x","y","z","s","sdg","t","tdg","sx","sxdg"};
  std::unordered_set<std::string> one_qbit_ro = {"ry","rz","rx","p"};
  std::unordered_set<std::string> two_qbit = {"cx","cy","cz","swap","csx","ch","dcx","ecr","cs","iswap","csdg"};
  std::unordered_set<std::string> two_qbit_ro = {"crx","cry","crz","cp","rzz","rxx","rzx","ryy"};
  std::unordered_set<std::string> three_qbit = {"ccx","cswap","rccx","ccz"};
  std::unordered_set<std::string> u_qbit = {"u"};
  std::unordered_set<std::string> cu_qbit = {"cu"};
  std::unordered_set<std::string> rc3x_qbit = {"rcccx"};
  std::unordered_set<std::string> rgate = {"r"};
  std::unordered_set<std::string> mcx_gate = {"mcx"};
  std::unordered_set<std::string> m_ro_gate = {"mcrx","mcry","mcrz","mcp"};
  std::unordered_set<std::string> ms_gate = {"ms"};
  
  if (gate_set.find(name) != gate_set.end()) {
    std::vector<std::string> qreg_names, qubit_symbol_table_keys;
    std::vector<mlir::Value> qbit_values, param_values;
    std::vector<mlir::Value> returnedValues;
    // {"h","x","y","z","s","sdg","t","tdg","sx","sxdg"}, 
    // {"cx","cy","cz","swap","csx","ch","dcx","ecr","cs","iswap","csdg"}, 
    // {"ccx","cswap","rccx","ccz"},
    // {"rcccx"}
    if((one_qbit.find(name) != one_qbit.end() && arg_size == 1) || (two_qbit.find(name) != two_qbit.end() && arg_size == 2) || 
    (three_qbit.find(name) != three_qbit.end()&& arg_size == 3) || (rc3x_qbit.find(name) != rc3x_qbit.end() && arg_size == 4)){
      mlir::Value value;
      for(auto idx_str_: context->gateArgumentList()->expression()){
        qreg_names.push_back("q");
        auto idx_str = idx_str_->getText();
        const auto idx_val = std::stoi(idx_str);
        value = get_or_extract_qubit("q", idx_val, location,
                                    symbol_table, builder);

        const auto qubit_symbol_name =
          symbol_table.array_qubit_symbol_name("q", idx_str);
        
        qbit_values.push_back(value);
        qubit_symbol_table_keys.push_back(qubit_symbol_name);
      }
    }else if(one_qbit_ro.find(name) != one_qbit_ro.end() && arg_size==2){ //{"ry","rz","rx","p"};
      qreg_names.push_back("q");
      auto expression_list = context->gateArgumentList()->expression();
      auto idx_str = expression_list.back()->getText();
      expression_list.pop_back();
      get_param_values(expression_list,param_values,builder,symbol_table,file_name,context,location);

      // auto idx_str = context->gateArgumentList()->expression(1)->getText();
      const auto qubit_symbol_name =
        symbol_table.array_qubit_symbol_name("q", idx_str);
      mlir::Value value;
      
      const auto idx_val = std::stoi(idx_str);
      value = get_or_extract_qubit("q", idx_val, location,
                                  symbol_table, builder);
      
      qbit_values.push_back(value);
      qubit_symbol_table_keys.push_back(qubit_symbol_name);
      
    }else if(u_qbit.find(name) != u_qbit.end() && arg_size==4){ //{"u"};
      mlir::Value value;
      auto expression_list = context->gateArgumentList()->expression();
      auto idx_str_ = expression_list.back();
      qreg_names.push_back("q");
      auto idx_str = idx_str_->getText();
      const auto idx_val = std::stoi(idx_str);
      value = get_or_extract_qubit("q", idx_val, location,
                                  symbol_table, builder);

      const auto qubit_symbol_name =
        symbol_table.array_qubit_symbol_name("q", idx_str);
      
      qbit_values.push_back(value);
      qubit_symbol_table_keys.push_back(qubit_symbol_name);

      expression_list.pop_back();
      get_param_values(expression_list,param_values,builder,symbol_table,file_name,context,location);
      

    }else if(two_qbit_ro.find(name) != two_qbit_ro.end() && arg_size==3){ //{"crx","cry","crz","cp","rzz","rxx","rzx","ryy"};
      mlir::Value value;
      auto expression_list = context->gateArgumentList()->expression();
      std::vector<qiskitParser::ExpressionContext *> param_list;
      param_list.emplace_back(expression_list.front());
      get_param_values(param_list,param_values,builder,symbol_table,file_name,context,location);

      expression_list.erase(expression_list.begin());
      for(auto idx_str_: expression_list){
        qreg_names.push_back("q");
        auto idx_str = idx_str_->getText();
        const auto idx_val = std::stoi(idx_str);
        value = get_or_extract_qubit("q", idx_val, location,
                                    symbol_table, builder);

        const auto qubit_symbol_name =
          symbol_table.array_qubit_symbol_name("q", idx_str);
        
        qbit_values.push_back(value);
        qubit_symbol_table_keys.push_back(qubit_symbol_name);
      }

    }else if(cu_qbit.find(name) != cu_qbit.end() && arg_size==5){ //{"cu"}
      mlir::Value value;
      auto expression_list = context->gateArgumentList()->expression();
      std::vector<qiskitParser::ExpressionContext *> lastTwo(expression_list.end() - 2, expression_list.end());
      expression_list.resize(expression_list.size() - 2);
      get_param_values(expression_list,param_values,builder,symbol_table,file_name,context,location);

      for(auto idx_str_: lastTwo){
        qreg_names.push_back("q");
        auto idx_str = idx_str_->getText();
        const auto idx_val = std::stoi(idx_str);
        value = get_or_extract_qubit("q", idx_val, location,
                                    symbol_table, builder);

        const auto qubit_symbol_name =
          symbol_table.array_qubit_symbol_name("q", idx_str);
        
        qbit_values.push_back(value);
        qubit_symbol_table_keys.push_back(qubit_symbol_name);
      }

    }else if(mcx_gate.find(name) != mcx_gate.end()&& arg_size==2){ //{"mcx"}
      auto expression_list = context->gateArgumentList()->expression();
      auto control_bit = expression_list.front();
      auto idx_str_ = expression_list.back();
      mlir::Value value;
      qiskit_expression_generator qubit_exp_generator(builder, symbol_table,
                                                     file_name);
      qubit_exp_generator.visit(control_bit);
      auto control_int = qubit_exp_generator.control_index;
      for(auto c_b: control_int){
        qreg_names.push_back("q");
        value = get_or_extract_qubit("q", c_b, location,
                                    symbol_table, builder);

        const auto qubit_symbol_name =
          symbol_table.array_qubit_symbol_name("q", std::to_string(c_b));
        
        qbit_values.push_back(value);
        qubit_symbol_table_keys.push_back(qubit_symbol_name);
      }

      qreg_names.push_back("q");
      auto idx_str = idx_str_->getText();
      const auto idx_val = std::stoi(idx_str);
      value = get_or_extract_qubit("q", idx_val, location,
                                  symbol_table, builder);

      const auto qubit_symbol_name =
        symbol_table.array_qubit_symbol_name("q", idx_str);
      
      qbit_values.push_back(value);
      qubit_symbol_table_keys.push_back(qubit_symbol_name);
    
    }else if(m_ro_gate.find(name) != m_ro_gate.end()&& arg_size==3){ //{"mcrx","mcry","mcrz","mcp"}
      auto expression_list = context->gateArgumentList()->expression();
      auto param = expression_list.front();
      auto control_bit = expression_list[1];
      auto idx_str_ = expression_list.back();
      std::vector<qiskitParser::ExpressionContext *> param_list;
      param_list.emplace_back(param);
      get_param_values(param_list,param_values,builder,symbol_table,file_name,context,location);


      mlir::Value value;
      qiskit_expression_generator qubit_exp_generator(builder, symbol_table,
                                                     file_name);
      qubit_exp_generator.visit(control_bit);
      auto control_int = qubit_exp_generator.control_index;
      for(auto c_b: control_int){
        qreg_names.push_back("q");
        value = get_or_extract_qubit("q", c_b, location,
                                    symbol_table, builder);

        const auto qubit_symbol_name =
          symbol_table.array_qubit_symbol_name("q", std::to_string(c_b));
        
        qbit_values.push_back(value);
        qubit_symbol_table_keys.push_back(qubit_symbol_name);
      }

      qreg_names.push_back("q");
      auto idx_str = idx_str_->getText();
      const auto idx_val = std::stoi(idx_str);
      value = get_or_extract_qubit("q", idx_val, location,
                                  symbol_table, builder);

      const auto qubit_symbol_name =
        symbol_table.array_qubit_symbol_name("q", idx_str);
      
      qbit_values.push_back(value);
      qubit_symbol_table_keys.push_back(qubit_symbol_name);
    
    }else if(ms_gate.find(name) != ms_gate.end()&& arg_size==2){ //{"ms"}
      auto expression_list = context->gateArgumentList()->expression();
      auto param = expression_list.front();
      auto control_bit = expression_list.back();
      std::vector<qiskitParser::ExpressionContext *> param_list;
      param_list.emplace_back(param);
      get_param_values(param_list,param_values,builder,symbol_table,file_name,context,location);

      mlir::Value value;
      qiskit_expression_generator qubit_exp_generator(builder, symbol_table,
                                                     file_name);
      qubit_exp_generator.visit(control_bit);
      auto control_int = qubit_exp_generator.control_index;
      for(auto c_b: control_int){
        qreg_names.push_back("q");
        value = get_or_extract_qubit("q", c_b, location,
                                    symbol_table, builder);

        const auto qubit_symbol_name =
          symbol_table.array_qubit_symbol_name("q", std::to_string(c_b));
        
        qbit_values.push_back(value);
        qubit_symbol_table_keys.push_back(qubit_symbol_name);
      }    
    }else{
      printErrorMessage("Wrong gate format .",
                              context->quantumGateName());
    }

    if(name == "u"){
      name = "u3";
    }
    if(name == "rcccx"){
      name = "rc3x";
    }
    returnedValues = createInstOps_HandleBroadcast(name, qbit_values, qreg_names,
                                  qubit_symbol_table_keys, param_values,
                                  location, context);
  }else{
    printErrorMessage("Not support .",
                            context->quantumGateName());
  }

  return 0;
}


}  // namespace qllvm