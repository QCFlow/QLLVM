
// Generated from qiskit.g4 by ANTLR 4.9.2

#pragma once


#include "antlr4-runtime.h"
#include "qiskitVisitor.h"


namespace qiskit {

/**
 * This class provides an empty implementation of qiskitVisitor, which can be
 * extended to create a visitor which only needs to handle a subset of the available methods.
 */
class  qiskitBaseVisitor : public qiskitVisitor {
public:

  virtual antlrcpp::Any visitProgram(qiskitParser::ProgramContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitQuantumCircuitDeclaration(qiskitParser::QuantumCircuitDeclarationContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitCircuitName(qiskitParser::CircuitNameContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitQuantumStatement(qiskitParser::QuantumStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitMeasurementStatement(qiskitParser::MeasurementStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitQuantumGateCall(qiskitParser::QuantumGateCallContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitQuantumGateName(qiskitParser::QuantumGateNameContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitQuantumMeasurement(qiskitParser::QuantumMeasurementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitQuantumBarrier(qiskitParser::QuantumBarrierContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitGateArgumentList(qiskitParser::GateArgumentListContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitMeasurementArgumentList(qiskitParser::MeasurementArgumentListContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitExpression(qiskitParser::ExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitExpressionTerminator(qiskitParser::ExpressionTerminatorContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitListExpression(qiskitParser::ListExpressionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitIntegerMeasurementArgument(qiskitParser::IntegerMeasurementArgumentContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitQubitMeasurementArgument(qiskitParser::QubitMeasurementArgumentContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitQuantumOperation(qiskitParser::QuantumOperationContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitExpression_1(qiskitParser::Expression_1Context *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitTerm(qiskitParser::TermContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitFactor(qiskitParser::FactorContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitQuantumCount(qiskitParser::QuantumCountContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitClassicalCount(qiskitParser::ClassicalCountContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitQubitReference(qiskitParser::QubitReferenceContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitClassicalReference(qiskitParser::ClassicalReferenceContext *ctx) override {
    return visitChildren(ctx);
  }


};

}  // namespace qiskit
