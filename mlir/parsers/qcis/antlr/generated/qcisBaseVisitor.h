
// Generated from qcis.g4 by ANTLR 4.9.2

#pragma once


#include "antlr4-runtime.h"
#include "qcisVisitor.h"


namespace qcis {

/**
 * This class provides an empty implementation of qcisVisitor, which can be
 * extended to create a visitor which only needs to handle a subset of the available methods.
 */
class  qcisBaseVisitor : public qcisVisitor {
public:

  virtual antlrcpp::Any visitProgram(qcisParser::ProgramContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitQuantumCircuitDeclaration(qcisParser::QuantumCircuitDeclarationContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitCircuitName(qcisParser::CircuitNameContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitQuantumStatement(qcisParser::QuantumStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitMeasurementStatement(qcisParser::MeasurementStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitQuantumGateCall(qcisParser::QuantumGateCallContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitQuantumBarrier(qcisParser::QuantumBarrierContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitQuantumGateName(qcisParser::QuantumGateNameContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitQuantumMeasurement(qcisParser::QuantumMeasurementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitGateArgumentList(qcisParser::GateArgumentListContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitMeasurementArgumentList(qcisParser::MeasurementArgumentListContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitExpressionArgument(qcisParser::ExpressionArgumentContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitIntegerGateArgument(qcisParser::IntegerGateArgumentContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitExpression(qcisParser::ExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitExpressionTerminator(qcisParser::ExpressionTerminatorContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitListExpression(qcisParser::ListExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitIntegerMeasurementArgument(qcisParser::IntegerMeasurementArgumentContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitQubitMeasurementArgument(qcisParser::QubitMeasurementArgumentContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitQuantumCount(qcisParser::QuantumCountContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitClassicalCount(qcisParser::ClassicalCountContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitQubitReference(qcisParser::QubitReferenceContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitGateParameter(qcisParser::GateParameterContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitClassicalReference(qcisParser::ClassicalReferenceContext *ctx) override {
    return visitChildren(ctx);
  }


};

}  // namespace qcis
