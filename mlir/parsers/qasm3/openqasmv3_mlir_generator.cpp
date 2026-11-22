/*******************************************************************************
 * Copyright (c) 2018-, UT-Battelle, LLC.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the MIT License
 * which accompanies this distribution.
 *
 * Contributors:
 *   Alexander J. McCaskey - initial API and implementation
 *   Thien Nguyen - implementation
 *
 * Modified by QCFlow (2026) for QLLVM project.
 *******************************************************************************/
#include "openqasmv3_mlir_generator.hpp"
#include <cctype>
#include <chrono>

#include "mlir/Dialect/StandardOps/IR/Ops.h"
#include "qasm3Lexer.h"
#include "qasm3Parser.h"
#include "qasm3_visitor.hpp"

namespace qllvm {

void OpenQasmV3MLIRGenerator::initialize_mlirgen(
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
  visitor = std::make_shared<qasm3_visitor>(builder, m_module, file_name);
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

void OpenQasmV3MLIRGenerator::initialize_mlirgen(
    bool _add_entry_point, const std::string function,
    std::map<std::string, std::string> extra_quantum_args) {
  file_name = function;
  traceFrontend = extra_quantum_args.count("_qllvm_trace_frontend");
  add_entry_point = _add_entry_point;

  // Only enable the rewrite to NISQ If-statements when the compilation 
  // targets NISQ qrt for some specific QPUs:
  // qasm-backend is intentionally excluded: emit goes through QIR → QirToQasm and
  // must use scf.if (not quantum::ConditionalOp + Callable), which lacks LLVM
  // globals for @if_body_*__body__wrapper on this path.
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

    if (!extra_quantum_args.count("qiskit_compat")) {
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

// Modified for correctness: this legacy lexer requires a decimal point in
// exponent literals. Normalize only numeric tokens, retaining identifiers,
// comments, and quoted include paths verbatim.
static std::string normalizeExponentLiterals(const std::string &source) {
  // Modified 2026-11-19: a linear token scan avoids constructing millions of
  // regex matches for generated benchmark circuits. Preserve quoted strings,
  // comments and identifier tokens exactly; only rewrite an integer mantissa
  // with a complete exponent (e.g. 12e-3 -> 12.0e-3).
  std::string result;
  result.reserve(source.size());
  auto digit = [](char ch) { return ch >= '0' && ch <= '9'; };
  auto alpha = [](char ch) {
    return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch == '_';
  };
  size_t i = 0;
  while (i < source.size()) {
    const size_t begin = i;
    if (source[i] == '"') {
      ++i;
      while (i < source.size()) {
        if (source[i] == '\\' && i + 1 < source.size()) { i += 2; continue; }
        if (source[i++] == '"') break;
      }
    } else if (source.compare(i, 2, "//") == 0) {
      auto end = source.find('\n', i + 2);
      i = end == std::string::npos ? source.size() : end;
    } else if (source.compare(i, 2, "/*") == 0) {
      auto end = source.find("*/", i + 2);
      i = end == std::string::npos ? source.size() : end + 2;
    } else if (alpha(source[i])) {
      while (i < source.size() && (alpha(source[i]) || digit(source[i]))) ++i;
    } else if (digit(source[i]) ||
               (source[i] == '.' && i + 1 < source.size() && digit(source[i+1]))) {
      while (i < source.size() && digit(source[i])) ++i;
      bool decimal = false;
      if (i < source.size() && source[i] == '.') {
        decimal = true;
        ++i;
        while (i < source.size() && digit(source[i])) ++i;
      }
      const size_t exponent = i;
      if (i < source.size() && (source[i] == 'e' || source[i] == 'E')) {
        size_t end = i + 1;
        if (end < source.size() && (source[end] == '+' || source[end] == '-')) ++end;
        const size_t digits = end;
        while (end < source.size() && digit(source[end])) ++end;
        if (end != digits) {
          i = end;
          if (!decimal) {
            result.append(source, begin, exponent - begin);
            result += ".0";
            result.append(source, exponent, i - exponent);
            continue;
          }
        }
      }
    } else {
      ++i;
    }
    result.append(source, begin, i - begin);
  }
  return result;
}

void OpenQasmV3MLIRGenerator::mlirgen(const std::string &src) {
  using namespace antlr4;
  using namespace qasm3;
  using Clock = std::chrono::steady_clock;
  const auto frontendStart = Clock::now();
  auto previous = frontendStart;
  auto mark = [&](const char *phase) {
    if (!traceFrontend) return;
    const auto now = Clock::now();
    std::cerr << "[qllvm-frontend] phase=" << phase << " seconds="
              << std::chrono::duration<double>(now-previous).count() << "\n";
    previous = now;
  };

  if (!visitor) {
    visitor = std::make_shared<qasm3_visitor>(builder, m_module, file_name,
                                              enable_qir_apply_ifelse);
  }
#ifndef ZHUYU_202410
  // Make sure we have the preamble text
  
  std::string preamble = "OPENQASM 3;";
  auto preamble_start = src.find(preamble);
  if (preamble_start == std::string::npos) {
    std::cout << "[OpenQASM MLIR Gen] Error, no OPENQASM 2.0 preamble text.\n";
    exit(1);
  }

  preamble = "include \"qelib1.inc\";";
  preamble_start = src.find(preamble);
  if (preamble_start == std::string::npos) {
    std::cout << "[OpenQASM MLIR Gen] Error, no include \"qelib1.inc\" "
                 "preamble text.\n";
    exit(1);
  }
  // Modified 2026-11-21: the old loop only trimmed leading whitespace and
  // rebuilt the same text while retaining a vector copy of every line. Gate
  // names and the copied source were unused. Let the lexer handle whitespace,
  // retaining comments/newlines and the required exponent normalization.
  const std::string src_new = normalizeExponentLiterals(src);
  ANTLRInputStream input(src_new);
#else
  ANTLRInputStream input(src);
#endif
  qasm3Lexer lexer(&input);
  CommonTokenStream tokens(&lexer);
  qasm3Parser parser(&tokens);
  mark("preprocess");

  class Qasm3ParserErrorListener : public BaseErrorListener {
   private:
    std::string file_name;

   public:
    Qasm3ParserErrorListener(const std::string &s) : file_name(s) {}
    void syntaxError(Recognizer *recognizer, Token *offendingSymbol,
                     size_t line, size_t charPositionInLine,
                     const std::string &msg, std::exception_ptr e) override {
      std::stringstream ss;
      ss << "\n\n[OPENQASM MLIRGen] cannot parse the source.\nerror: " << msg
         << "\n";
      ss << "occurred at " << file_name << ":" << line << ":"
         << charPositionInLine
         << ", offending symbol = "
         << (offendingSymbol ? offendingSymbol->getText() : "<lexer>") << "\n\n";
      std::cout << ss.str();
      exit(1);
    }
  };

  // Setup our custom error listeners
  lexer.removeErrorListeners();
  parser.removeErrorListeners();
  Qasm3ParserErrorListener el(file_name);
  lexer.addErrorListener(&el);
  if (traceFrontend) {
    tokens.fill(); // Explicitly separate lexer cost only in diagnostic mode.
    mark("tokenize");
  }

  // Modified 2026-11-19: avoid full-context prediction for every gate in
  // long flat circuits. Retry from the beginning in LL mode on an SLL
  // ambiguity; never visit a partial tree or recover silently from bad input.
  tree::ParseTree *tree = nullptr;
  parser.getInterpreter<atn::ParserATNSimulator>()->setPredictionMode(
      atn::PredictionMode::SLL);
  parser.setErrorHandler(std::make_shared<BailErrorStrategy>());
  try {
    tree = parser.program();
    if (tokens.LA(1) != Token::EOF)
      throw ParseCancellationException("Trailing tokens after program");
  } catch (const ParseCancellationException &) {
    tokens.seek(0);
    parser.reset();
    parser.getInterpreter<atn::ParserATNSimulator>()->setPredictionMode(
        atn::PredictionMode::LL);
    parser.setErrorHandler(std::make_shared<DefaultErrorStrategy>());
    parser.addErrorListener(&el);
    tree = parser.program();
  }
  if (tokens.LA(1) != Token::EOF) {
    auto *token = tokens.LT(1);
    el.syntaxError(&parser, token, token->getLine(),
                   token->getCharPositionInLine(),
                   "Unexpected trailing token after program", nullptr);
  }
  mark("parse");
  visitor->visitChildren(tree);
  mark("mlir-generate");
  return;
}

void OpenQasmV3MLIRGenerator::finalize_mlirgen() {
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
