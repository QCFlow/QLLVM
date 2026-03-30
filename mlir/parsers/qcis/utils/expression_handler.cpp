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

#include "expression_handler.hpp"
#include "mlir/Dialect/SCF/SCF.h"

using namespace qcis;

namespace qllvm {

void qcis_expression_generator::update_current_value(mlir::Value v) {
  last_current_value = current_value;
  current_value = v;
  return;
}

qcis_expression_generator::qcis_expression_generator(mlir::OpBuilder& b,
                                                       ScopedSymbolTable& table,
                                                       std::string& fname)
    : builder(b), file_name(fname), symbol_table(table) {
  internal_value_type = builder.getI64Type();
}

qcis_expression_generator::qcis_expression_generator(mlir::OpBuilder& b,
                                                       ScopedSymbolTable& table,
                                                       std::string& fname,
                                                       mlir::Type t)
    : builder(b),
      file_name(fname),
      symbol_table(table),
      internal_value_type(t) {}

antlrcpp::Any qcis_expression_generator::visitTerminal(
    antlr4::tree::TerminalNode* node) {
  auto location = builder.getUnknownLoc();  //(builder, file_name, ctx);
  if (node->getSymbol()->getText() == "[") {
    // We have hit a closing on an index
    // std::cout << "TERMNODE:\n";
    indexed_variable_value = current_value;
    if (casting_indexed_integer_to_bool) {
      if (indexed_variable_value.getType().isa<mlir::MemRefType>()) {
        internal_value_type = indexed_variable_value.getType()
                                  .cast<mlir::MemRefType>()
                                  .getElementType();
      } else {
        internal_value_type = builder.getIndexType();
      }
    } else if (indexed_variable_value.getType().isa<mlir::MemRefType>()) {
      internal_value_type = builder.getIndexType();
    }

    // Extract the variable name VARNAME[IDX] by searching parent children
    auto c = node->parent->children;
    auto it = std::find(c.begin(), c.end(), node);
    auto index = std::distance(c.begin(), it) - 1;
    indexed_variable_name = node->parent->children[index]->getText();
  } else if (node->getSymbol()->getText() == "]") {
    if (casting_indexed_integer_to_bool) {
      mlir::IntegerType signless_integer_like_type =
          builder.getIntegerType(indexed_variable_value.getType()
                                     .cast<mlir::MemRefType>()
                                     .getElementType()
                                     .getIntOrFloatBitWidth());
      auto casted_idx =
          builder
              .create<mlir::quantum::IntegerCastOp>(
                  location, signless_integer_like_type, current_value)
              .output();
      auto load_value =
          builder.create<mlir::LoadOp>(location, indexed_variable_value);

      auto load_value_casted =
          builder
              .create<mlir::quantum::IntegerCastOp>(
                  location, signless_integer_like_type, load_value)
              .output();
      // Note: 'std.shift_right_unsigned' op requires the same type for
      // all operands and results
      assert(load_value_casted.getType() == casted_idx.getType());
      auto shift = builder.create<mlir::UnsignedShiftRightOp>(
          location, load_value_casted, casted_idx);

      auto and_value = builder.create<mlir::AndOp>(
          location, shift,
          get_or_create_constant_integer_value(
              1, location, signless_integer_like_type, symbol_table, builder));
      update_current_value(and_value.result());
      casting_indexed_integer_to_bool = false;
    } else {
      if (internal_value_type.dyn_cast_or_null<mlir::OpaqueType>() &&
          internal_value_type.cast<mlir::OpaqueType>().getTypeData().str() ==
              "Qubit") {
        if (current_value.getType().isa<mlir::MemRefType>()) {
          if (current_value.getType().cast<mlir::MemRefType>().getRank() == 0) {
            current_value =
                builder.create<mlir::LoadOp>(location, current_value);
          } else {
            printErrorMessage("Terminator ']' -> Invalid qubit array index: ",
                              current_value);
          }
        }
        if (current_value.getType().getIntOrFloatBitWidth() < 64) {
          current_value = builder.create<mlir::ZeroExtendIOp>(
              location, current_value, builder.getI64Type());
        }
        if (!current_value.getType().isa<mlir::IntegerType>()) {
          current_value = builder.create<mlir::IndexCastOp>(
              location, builder.getI64Type(), current_value);
        }
        
        // This is a qubit extract from qubit array:
        // Need to use the qreg name convention:
        // i.e. qreg%index
        if (auto constantOp =
                current_value.getDefiningOp<mlir::ConstantOp>()) {
          // If the index is a constant value:
          const auto index_val =
              constantOp.getValue().cast<mlir::IntegerAttr>().getInt();
          const std::string qreg_name = symbol_table.get_symbol_var_name(indexed_variable_value);
          // If cannot lookup, this indicates a logic error (failed to add the qreg to the symbol table).
          assert(!qreg_name.empty());
          mlir::Value extracted_qubit = get_or_extract_qubit(
              qreg_name, index_val, location, symbol_table, builder);
          update_current_value(extracted_qubit);
        } else {
          // We're getting accessing qubits at unknown indices...
          // (current_value cannot be const-eval'ed)
          const std::string qreg_name =
              symbol_table.get_symbol_var_name(indexed_variable_value);
          symbol_table.invalidate_qubit_extracts(qreg_name);

          update_current_value(builder.create<mlir::quantum::ExtractQubitOp>(
              location, get_custom_opaque_type("Qubit", builder.getContext()),
              indexed_variable_value, current_value));
        }
      } else if (indexed_variable_value.getType().isa<mlir::OpaqueType>() &&
                 indexed_variable_value.getType()
                         .cast<mlir::OpaqueType>()
                         .getTypeData()
                         .str() == "Array") {
        auto attrs =
            symbol_table.get_variable_attributes(indexed_variable_name);
        if (attrs.empty()) {
          printErrorMessage(
              "To index a non-qubit Array we have to know the type via the "
              "variable attributes in the symbol table.");
        }
        auto attribute = attrs[0];
        mlir::Type array_type;
        if (attribute == "double") {
          array_type = builder.getF64Type();
        } else {
          printErrorMessage("We do not support this array type yet: " +
                            attribute);
        }

        update_current_value(
            builder.create<mlir::quantum::GeneralArrayExtractOp>(
                location, array_type, indexed_variable_value,
                cast_array_index_value_if_required(
                    indexed_variable_value.getType(), current_value, location,
                    builder)));
        indexed_variable_name = "";
      } else {
        if (current_value.getType().isa<mlir::MemRefType>()) {
          current_value = builder.create<mlir::LoadOp>(location, current_value);
        }

        llvm::ArrayRef<mlir::Value> idx(cast_array_index_value_if_required(
            indexed_variable_value.getType(), current_value, location,
            builder));
        update_current_value(builder.create<mlir::LoadOp>(
            location, indexed_variable_value, idx));
      }
    }
  }
  return 0;
}

antlrcpp::Any qcis_expression_generator::visitExpression(
    qcisParser::ExpressionContext* ctx) {
  return visitChildren(ctx);
}


// expressionTerminator
//     : Constant
//     | Integer
//     | RealNumber
//     | Identifier
//     | StringLiteral
//     | listExpression
//     | MINUS expressionTerminator
//     ;

// listExpression
//     : LBRACKET gateArgumentList RBRACKET
//     ;
antlrcpp::Any qcis_expression_generator::visitExpressionTerminator(
    qcisParser::ExpressionTerminatorContext* ctx) {
  auto location = get_location(builder, file_name, ctx);

  // std::cout << "Analyze Expression Terminator: " << ctx->getText() << "\n";

  int multiplier = 1;
  if (ctx->MINUS() && ctx->expressionTerminator()) {
    visit(ctx->expressionTerminator());
    if (current_value.getType().isIntOrFloat()) {
      mlir::Attribute attr;
      if (current_value.getType().isa<mlir::FloatType>()) {
        attr = mlir::FloatAttr::get(builder.getF64Type(), -1.0);
      } else {
        attr = mlir::IntegerAttr::get(builder.getI64Type(), -1);
      }
      auto const_op = builder.create<mlir::ConstantOp>(location, attr);
      if (current_value.getType().isa<mlir::FloatType>()) {
        createOp<mlir::MulFOp>(location, const_op, current_value);
      } else {
        createOp<mlir::MulIOp>(location, const_op, current_value);
      }
    }
    return 0;
  }

  if (ctx->Constant()) {
    auto const_str = ctx->Constant()->getText();
    // std::cout << ctx->Constant()->getText() << "\n";
    double multiplier = ctx->MINUS() ? -1 : 1;
    double constant_val = 0.0;
    if (const_str == "pi") {
      constant_val = pi;
    } else {
      printErrorMessage("Constant " + const_str + " not implemented yet.");
    }
    auto value = multiplier * constant_val;
    auto float_attr = mlir::FloatAttr::get(builder.getF64Type(), value);
    createOp<mlir::ConstantOp>(location, float_attr);
    return 0;
  } else if (auto integer = ctx->Integer()) {
    // check minus
    int multiplier = ctx->MINUS() ? -1 : 1;
    if (builtin_math_func_treat_ints_as_float) {
      auto value = std::stod(integer->getText());
      createOp<mlir::ConstantOp>(
          location, mlir::FloatAttr::get(builder.getF64Type(), value));
    } else {
      auto idx = std::stoi(integer->getText());
      // std::cout << "Integer Terminator " << integer->getText() << ", " << idx
      // << "\n";
      const auto getSignlessIntegerType = [](mlir::OpBuilder &opBuilder,
                                             mlir::IntegerType in_intType) {
        return in_intType.isSignless()
                   ? in_intType
                   : opBuilder.getIntegerType(in_intType.getWidth());
      };
      auto integer_attr = mlir::IntegerAttr::get(
          (internal_value_type.dyn_cast_or_null<mlir::IntegerType>()
               ? getSignlessIntegerType(
                     builder, internal_value_type.cast<mlir::IntegerType>())
               : builder.getI64Type()),
          idx);

      assert(integer_attr.getType().cast<mlir::IntegerType>().isSignless());
      if (internal_value_type.dyn_cast_or_null<mlir::IntegerType>() &&
          !internal_value_type.cast<mlir::IntegerType>().isSignless()) {
        // Make sure we cast the constant value appropriately.
        // i.e. respect the signed/unsigned of the requested type.
        current_value = builder.create<mlir::quantum::IntegerCastOp>(
            location, internal_value_type.cast<mlir::IntegerType>(),
            builder.create<mlir::ConstantOp>(location, integer_attr)).output();
      } else {
        current_value =
            builder.create<mlir::ConstantOp>(location, integer_attr);
      }
    }
    return 0;
  } else if (auto real = ctx->RealNumber()) {
    // check minus
    double multiplier = ctx->MINUS() ? -1 : 1;
    auto value = multiplier * std::stod(real->getText());
    auto float_attr = mlir::FloatAttr::get(
        (internal_value_type.dyn_cast_or_null<mlir::FloatType>()
             ? internal_value_type.cast<mlir::FloatType>()
             : builder.getF64Type()),
        value);
    createOp<mlir::ConstantOp>(location, float_attr);
    return 0;
  } else if (auto id = ctx->Identifier()) {
    // std::cout << "Getting reference to variable " << id->getText() << "\n";
    mlir::Value value;
    if (id->getText() == "True") {
      value = get_or_create_constant_integer_value(
          1, location, builder.getIntegerType(1), symbol_table, builder);
    } else if (id->getText() == "False") {
      value = get_or_create_constant_integer_value(
          0, location, builder.getIntegerType(1), symbol_table, builder);
    } else {
      value = symbol_table.get_symbol(id->getText());
      // If we are not in global scope and this value is
      // marked const, then I want to re-create it and return
      // that, this will mimic using global constants in downstream
      // scopes
      // if (symbol_table.get_current_scope() != 0) {
      //   auto var_attrs = symbol_table.get_variable_attributes(id->getText());
      //   if (!var_attrs.empty() && std::find(var_attrs.begin(),
      //   var_attrs.end(),
      //                                       "const") != std::end(var_attrs))
      //                                       {
      //     auto constant_val =
      //     value.getDefiningOp<mlir::ConstantOp>().value(); value =
      //     builder.create<mlir::ConstantOp>(location, constant_val);
      //   }
      // }
    }
    update_current_value(value);

    return 0;
  } else if (auto listExpr = ctx->listExpression()) {
    auto integer_nodes = listExpr->Integer();
    control_index.clear();
    // std::cout << "integer_nodes.size(): " << integer_nodes.size() << std::endl;
    for(auto integer_s: integer_nodes){
      auto idx = std::stoi(integer_s->getText());
      control_index.emplace_back(idx);
    }
    return 0;

  } else if (ctx->StringLiteral()) {
    auto sl = ctx->StringLiteral()->getText();
    sl = sl.substr(1, sl.length() - 2);
    llvm::StringRef string_type_name("StringType");
    mlir::Identifier dialect =
        mlir::Identifier::get("quantum", builder.getContext());
    auto str_type =
        mlir::OpaqueType::get(builder.getContext(), dialect, string_type_name);
    auto str_attr = builder.getStringAttr(sl);

    std::hash<std::string> hasher;
    auto hash = hasher(sl);
    std::stringstream ss;
    ss << "__internal_string_literal__" << hash;
    std::string var_name = ss.str();
    auto var_name_attr = builder.getStringAttr(var_name);

    update_current_value(builder.create<mlir::quantum::CreateStringLiteralOp>(
        location, str_type, str_attr, var_name_attr));
    return 0;

  }else {
    printErrorMessage("Cannot handle this expression terminator yet: " +
                      ctx->getText());
  }

  return 0;
}

}  // namespace qllvm
