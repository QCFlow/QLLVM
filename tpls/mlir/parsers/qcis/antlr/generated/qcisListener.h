
// Generated from qcis.g4 by ANTLR 4.9.2

#pragma once


#include "antlr4-runtime.h"
#include "qcisParser.h"


namespace qcis {

/**
 * This interface defines an abstract listener for a parse tree produced by qcisParser.
 */
class  qcisListener : public antlr4::tree::ParseTreeListener {
public:

  virtual void enterProgram(qcisParser::ProgramContext *ctx) = 0;
  virtual void exitProgram(qcisParser::ProgramContext *ctx) = 0;

  virtual void enterQuantumCircuitDeclaration(qcisParser::QuantumCircuitDeclarationContext *ctx) = 0;
  virtual void exitQuantumCircuitDeclaration(qcisParser::QuantumCircuitDeclarationContext *ctx) = 0;

  virtual void enterCircuitName(qcisParser::CircuitNameContext *ctx) = 0;
  virtual void exitCircuitName(qcisParser::CircuitNameContext *ctx) = 0;

  virtual void enterQuantumStatement(qcisParser::QuantumStatementContext *ctx) = 0;
  virtual void exitQuantumStatement(qcisParser::QuantumStatementContext *ctx) = 0;

  virtual void enterMeasurementStatement(qcisParser::MeasurementStatementContext *ctx) = 0;
  virtual void exitMeasurementStatement(qcisParser::MeasurementStatementContext *ctx) = 0;

  virtual void enterQuantumGateCall(qcisParser::QuantumGateCallContext *ctx) = 0;
  virtual void exitQuantumGateCall(qcisParser::QuantumGateCallContext *ctx) = 0;

  virtual void enterQuantumBarrier(qcisParser::QuantumBarrierContext *ctx) = 0;
  virtual void exitQuantumBarrier(qcisParser::QuantumBarrierContext *ctx) = 0;

  virtual void enterQuantumGateName(qcisParser::QuantumGateNameContext *ctx) = 0;
  virtual void exitQuantumGateName(qcisParser::QuantumGateNameContext *ctx) = 0;

  virtual void enterQuantumMeasurement(qcisParser::QuantumMeasurementContext *ctx) = 0;
  virtual void exitQuantumMeasurement(qcisParser::QuantumMeasurementContext *ctx) = 0;

  virtual void enterGateArgumentList(qcisParser::GateArgumentListContext *ctx) = 0;
  virtual void exitGateArgumentList(qcisParser::GateArgumentListContext *ctx) = 0;

  virtual void enterMeasurementArgumentList(qcisParser::MeasurementArgumentListContext *ctx) = 0;
  virtual void exitMeasurementArgumentList(qcisParser::MeasurementArgumentListContext *ctx) = 0;

  virtual void enterExpressionArgument(qcisParser::ExpressionArgumentContext *ctx) = 0;
  virtual void exitExpressionArgument(qcisParser::ExpressionArgumentContext *ctx) = 0;

  virtual void enterIntegerGateArgument(qcisParser::IntegerGateArgumentContext *ctx) = 0;
  virtual void exitIntegerGateArgument(qcisParser::IntegerGateArgumentContext *ctx) = 0;

  virtual void enterExpression(qcisParser::ExpressionContext *ctx) = 0;
  virtual void exitExpression(qcisParser::ExpressionContext *ctx) = 0;

  virtual void enterExpressionTerminator(qcisParser::ExpressionTerminatorContext *ctx) = 0;
  virtual void exitExpressionTerminator(qcisParser::ExpressionTerminatorContext *ctx) = 0;

  virtual void enterListExpression(qcisParser::ListExpressionContext *ctx) = 0;
  virtual void exitListExpression(qcisParser::ListExpressionContext *ctx) = 0;

  virtual void enterIntegerMeasurementArgument(qcisParser::IntegerMeasurementArgumentContext *ctx) = 0;
  virtual void exitIntegerMeasurementArgument(qcisParser::IntegerMeasurementArgumentContext *ctx) = 0;

  virtual void enterQubitMeasurementArgument(qcisParser::QubitMeasurementArgumentContext *ctx) = 0;
  virtual void exitQubitMeasurementArgument(qcisParser::QubitMeasurementArgumentContext *ctx) = 0;

  virtual void enterQuantumCount(qcisParser::QuantumCountContext *ctx) = 0;
  virtual void exitQuantumCount(qcisParser::QuantumCountContext *ctx) = 0;

  virtual void enterClassicalCount(qcisParser::ClassicalCountContext *ctx) = 0;
  virtual void exitClassicalCount(qcisParser::ClassicalCountContext *ctx) = 0;

  virtual void enterQubitReference(qcisParser::QubitReferenceContext *ctx) = 0;
  virtual void exitQubitReference(qcisParser::QubitReferenceContext *ctx) = 0;

  virtual void enterGateParameter(qcisParser::GateParameterContext *ctx) = 0;
  virtual void exitGateParameter(qcisParser::GateParameterContext *ctx) = 0;

  virtual void enterClassicalReference(qcisParser::ClassicalReferenceContext *ctx) = 0;
  virtual void exitClassicalReference(qcisParser::ClassicalReferenceContext *ctx) = 0;


};

}  // namespace qcis
