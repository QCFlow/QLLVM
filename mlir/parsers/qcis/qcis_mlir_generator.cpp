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

#include "qcis_mlir_generator.hpp"

#include <regex>

#include "mlir/Dialect/StandardOps/IR/Ops.h"
#include "qcis_visitor.hpp"
#include "antlr/generated/qcisLexer.h"
#include "antlr/generated/qcisParser.h"
#include "qasm3_utils.hpp"
namespace qllvm {

void qcisMLIRGenerator::initialize_mlirgen(
  const std::string func_name, std::vector<mlir::Type> arg_types,
  std::vector<std::string> arg_var_names,
  std::vector<std::string> var_attributes, mlir::Type return_type) {

  mlir::FunctionType func_type2;
  if (return_type) {
    func_type2 =
        builder.getFunctionType(llvm::makeArrayRef(arg_types), return_type);
  } else {
    func_type2 =
        builder.getFunctionType(llvm::makeArrayRef(arg_types), llvm::None);
  }
  auto proto2 = mlir::FuncOp::create(
      builder.getUnknownLoc(), "__internal_mlir_" + func_name, func_type2);
  mlir::FuncOp function2(proto2);
  std::string file_name = "internal_mlirgen_qllvm_";
  auto save_main_entry_block = function2.addEntryBlock();
  builder.setInsertionPointToStart(save_main_entry_block);
  m_module.push_back(function2);
  main_entry_block = save_main_entry_block;

  // Configure block arguments
  visitor = std::make_shared<qcis_visitor>(builder, m_module, file_name);
  auto symbol_table = visitor->getScopedSymbolTable();
  auto arguments = main_entry_block->getArguments();
  for (int i = 0; i < arg_var_names.size(); i++) {
    symbol_table->add_symbol(arg_var_names[i], arguments[i],
                             std::vector<std::string>{var_attributes[i]});
  }

  add_main = false;
  if (!return_type) {
    add_custom_return = true;
  }

  return;
  
}

void qcisMLIRGenerator::initialize_mlirgen(
    bool _add_entry_point, const std::string function,
    std::map<std::string, std::string> extra_quantum_args) {
  file_name = function;
  add_entry_point = _add_entry_point;

  // Only enable the rewrite to NISQ If-statements when the compilation 
  // targets NISQ qrt for some specific QPUs:
  static const std::vector<std::string> IF_STMT_CAPABLE_QPUS{"qpp", "aer",
                                                             "honeywell"};
  if (extra_quantum_args.find("qrt") != extra_quantum_args.end() &&
      extra_quantum_args["qrt"] == "nisq") {
    // Default is qpp (i.e., not provided)
    if (extra_quantum_args.find("qpu") == extra_quantum_args.end()) {
      enable_qir_apply_ifelse = true;
    } else {
      for (const auto &name_to_check : IF_STMT_CAPABLE_QPUS) {
        const auto qpu_name = extra_quantum_args["qpu"];
        if (qpu_name.rfind(name_to_check, 0) == 0) {
          // QPU start with aer, honeywell, etc.
          // (it could have backend name customization after ':')
          enable_qir_apply_ifelse = true;
          break;
        }
      }
    }
  }

  initCommonOpaqueTypes();
  auto int_type = builder.getI32Type();
  mlir::Identifier dialect = mlir::Identifier::get("quantum", &context);
  auto argv_type =
      mlir::OpaqueType::get(&context, dialect, llvm::StringRef("ArgvType"));
  auto qreg_type =
      mlir::OpaqueType::get(&context, dialect, llvm::StringRef("qreg"));

  if (add_main) {
    std::vector<mlir::Type> arg_types_vec2{};
    auto func_type2 = builder.getFunctionType(
        llvm::makeArrayRef(arg_types_vec2), builder.getI32Type());
    auto proto2 = mlir::FuncOp::create(
        builder.getUnknownLoc(), "__internal_mlir_" + file_name, func_type2);
    mlir::FuncOp function2(proto2);
    auto save_main_entry_block = function2.addEntryBlock();

    if (add_entry_point) {
      std::vector<mlir::Type> arg_types_vec{int_type, argv_type};
      auto func_type =
          builder.getFunctionType(llvm::makeArrayRef(arg_types_vec), int_type);

      std::vector<mlir::Attribute> main_attrs;
      for (auto &[k, v] : extra_quantum_args) {
        main_attrs.push_back(mlir::StringAttr::get(k, builder.getContext()));
        main_attrs.push_back(mlir::StringAttr::get(v, builder.getContext()));
      }

      mlir::ArrayRef<mlir::Attribute> extra_args_attr =
          llvm::makeArrayRef(main_attrs);

      auto proto =
          mlir::FuncOp::create(builder.getUnknownLoc(), "main", func_type);
      mlir::FuncOp function(proto);
      main_entry_block = function.addEntryBlock();
      auto &entryBlock = *main_entry_block;
      builder.setInsertionPointToStart(&entryBlock);

      auto main_args = main_entry_block->getArguments();
      builder.create<mlir::quantum::QRTInitOp>(
          builder.getUnknownLoc(), main_args[0], main_args[1],
          mlir::ArrayAttr::get(extra_args_attr, builder.getContext()));

      // call the function from main, run finalize, and return 0
      auto call_internal =
          builder.create<mlir::CallOp>(builder.getUnknownLoc(), function2);
      builder.create<mlir::quantum::QRTFinalizeOp>(builder.getUnknownLoc());

      builder.create<mlir::ReturnOp>(builder.getUnknownLoc(),
                                     call_internal.getResult(0));
      m_module.push_back(function);
      function_names.push_back("main");
    }

    m_module.push_back(function2);

    if (!extra_quantum_args.count("qcis_compat")) {
      std::vector<mlir::Type> arg_types_vec3{qreg_type};
      auto func_type3 = builder.getFunctionType(
          llvm::makeArrayRef(arg_types_vec3), builder.getI32Type());
      auto proto3 =
          mlir::FuncOp::create(builder.getUnknownLoc(), file_name, func_type3);
      mlir::FuncOp function3(proto3);

      auto tmp = function3.addEntryBlock();
      builder.setInsertionPointToStart(tmp);
      builder.create<mlir::quantum::SetQregOp>(builder.getUnknownLoc(),
                                               tmp->getArguments()[0]);
      auto call_internal =
          builder.create<mlir::CallOp>(builder.getUnknownLoc(), function2);
      builder.create<mlir::quantum::QRTFinalizeOp>(builder.getUnknownLoc());
      builder.create<mlir::ReturnOp>(
          builder.getUnknownLoc(),
          llvm::ArrayRef<mlir::Value>(call_internal.getResult(0)));
      builder.setInsertionPointToStart(save_main_entry_block);
      m_module.push_back(function3);
    } 

    function_names.push_back("__internal_mlir_" + file_name);
    function_names.push_back(file_name);

    main_entry_block = save_main_entry_block;
  }
}

void qcisMLIRGenerator::mlirgen(const std::string &src) {
  // std::cout << "this to gen mlir" << std::endl;
  using namespace antlr4;
  using namespace qcis;
  using namespace qasm3;

  if (!visitor) {
    visitor = std::make_shared<qcis_visitor>(builder, m_module, file_name,
                                              enable_qir_apply_ifelse);
  }

  ANTLRInputStream input(src);
  qcisLexer lexer(&input);
  CommonTokenStream tokens(&lexer);
  qcisParser parser(&tokens);

  lexer.removeErrorListeners();
  parser.removeErrorListeners();
  tree::ParseTree *tree = parser.program();

  // QCIS has no quantumCircuitDeclaration; infer and allocate q/c implicitly.
  auto symbol_table = visitor->getScopedSymbolTable();
  if (!symbol_table->has_symbol("q")) {
    int max_qubit_idx = -1;
    std::regex qubit_re("Q(\\d+)");
    for (std::sregex_iterator it(src.begin(), src.end(), qubit_re), end;
         it != end; ++it) {
      int idx = std::stoi((*it)[1].str());
      max_qubit_idx = std::max(max_qubit_idx, idx);
    }
    int64_t n_qubits = (max_qubit_idx >= 0) ? (max_qubit_idx + 1) : 1;

    auto location = builder.getUnknownLoc();
    auto integer_attr =
        mlir::IntegerAttr::get(builder.getI64Type(), n_qubits);
    auto str_attr = builder.getStringAttr("q");
    mlir::Value q_alloc = builder.create<mlir::quantum::QallocOp>(
        location, array_type, integer_attr, str_attr);
    symbol_table->add_global_symbol("q", q_alloc);

    bool has_measurements =
        (src.find("M ") != std::string::npos ||
         src.find("M\t") != std::string::npos);
    if (has_measurements) {
      int64_t n_cbits = n_qubits;
      std::vector<mlir::Value> init_values, init_indices;
      for (int64_t i = 0; i < n_cbits; i++) {
        init_values.push_back(get_or_create_constant_integer_value(
            0, location, builder.getI1Type(), *symbol_table, builder));
        init_indices.push_back(get_or_create_constant_index_value(
            i, location, 64, *symbol_table, builder));
      }
      if (n_cbits == 1) {
        llvm::ArrayRef<int64_t> shaperef{};
        auto mem_type =
            mlir::MemRefType::get(shaperef, builder.getI1Type());
        mlir::Value c_alloc = builder.create<mlir::AllocaOp>(location, mem_type);
        builder.create<mlir::StoreOp>(location, init_values[0], c_alloc);
        symbol_table->add_global_symbol("c", c_alloc);
      } else {
        llvm::ArrayRef<int64_t> shaperef(n_cbits);
        auto mem_type =
            mlir::MemRefType::get(shaperef, builder.getI1Type());
        mlir::Value c_alloc = builder.create<mlir::AllocaOp>(location, mem_type);
        for (int64_t i = 0; i < n_cbits; i++) {
          builder.create<mlir::StoreOp>(location, init_values[i], c_alloc,
                                        init_indices[i]);
        }
        symbol_table->add_global_symbol("c", c_alloc);
      }
    }
  }

  visitor->visitChildren(tree);
  // std::cout << "finish " << std::endl;
  return;
}

void qcisMLIRGenerator::finalize_mlirgen() {
  auto scoped_symbol_table = visitor->getScopedSymbolTable();
  if (auto b = scoped_symbol_table->get_last_created_block()) {
    builder.setInsertionPointToEnd(b);
  }
  auto all_qalloc_ops =
      scoped_symbol_table
          ->get_global_symbols_of_type<mlir::quantum::QallocOp>();
  for (auto op : all_qalloc_ops) {
    builder.create<mlir::quantum::DeallocOp>(builder.getUnknownLoc(), op);
  }

  // Add any function names that we created.
  auto fnames = scoped_symbol_table->get_seen_function_names();
  for (auto f : fnames) {
    function_names.push_back(f);
  }

  if (add_main) {
    if (auto b = scoped_symbol_table->get_last_created_block()) {
      builder.setInsertionPointToEnd(b);
    } else {
      builder.setInsertionPointToEnd(main_entry_block);
    }

    auto integer_attr = mlir::IntegerAttr::get(builder.getI32Type(), 0);
    auto ret =
        builder.create<mlir::ConstantOp>(builder.getUnknownLoc(), integer_attr);
    builder.create<mlir::ReturnOp>(builder.getUnknownLoc(),
                                   llvm::ArrayRef<mlir::Value>(ret));
  }

  if (add_custom_return) {
    builder.create<mlir::ReturnOp>(builder.getUnknownLoc(),
                                   llvm::ArrayRef<mlir::Value>());
  }
}

} // namespace qllvm
