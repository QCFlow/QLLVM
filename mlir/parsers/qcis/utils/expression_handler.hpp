
#pragma once
#include "Quantum/QuantumOps.h"
#include "mlir/Dialect/StandardOps/IR/Ops.h"
#include "mlir/IR/BuiltinOps.h"
#include "qcisBaseVisitor.h"
#include "qcisParser.h"
#include "qasm3_utils.hpp"
#include "symbol_table.hpp"

static constexpr double pi = 3.141592653589793238;
using namespace qcis;

namespace qllvm {
class qcis_expression_generator : public qcis::qcisBaseVisitor {
 protected:
  mlir::OpBuilder& builder;
  mlir::ModuleOp m_module;
  std::string file_name = "";

  std::string indexed_variable_name = "";
  
  bool builtin_math_func_treat_ints_as_float = false;
  bool casting_indexed_integer_to_bool = false;
  bool found_negation_unary_op = false;
  mlir::Value indexed_variable_value;

  mlir::Type internal_value_type;


  qllvm::ScopedSymbolTable& symbol_table;

  void update_current_value(mlir::Value v);

  template <typename OpTy, typename... Args>
  OpTy createOp(Args... args) {
    OpTy value = builder.create<OpTy>(args...);
    update_current_value(value);
    return value;
  }

 public:
  mlir::Value current_value;
  std::vector<int> control_index;
  mlir::Value last_current_value;

  qcis_expression_generator(mlir::OpBuilder& b, qllvm::ScopedSymbolTable& table,
                             std::string& fname);
  qcis_expression_generator(mlir::OpBuilder& b, qllvm::ScopedSymbolTable& table,
                             std::string& fname, mlir::Type t);

  antlrcpp::Any visitTerminal(antlr4::tree::TerminalNode* node) override;
  antlrcpp::Any visitExpression(qcisParser::ExpressionContext* ctx) override;

  antlrcpp::Any visitExpressionTerminator(
      qcisParser::ExpressionTerminatorContext* ctx) override;
};
}  // namespace qllvm