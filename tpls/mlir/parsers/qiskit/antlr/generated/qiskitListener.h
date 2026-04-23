
// Generated from qiskit.g4 by ANTLR 4.9.2

#pragma once


#include "antlr4-runtime.h"
#include "qiskitParser.h"


namespace qiskit {

/**
 * This interface defines an abstract listener for a parse tree produced by qiskitParser.
 */
class  qiskitListener : public antlr4::tree::ParseTreeListener {
public:

  virtual void enterProgram(qiskitParser::ProgramContext *ctx) = 0;
  virtual void exitProgram(qiskitParser::ProgramContext *ctx) = 0;

  virtual void enterQuantumCircuitDeclaration(qiskitParser::QuantumCircuitDeclarationContext *ctx) = 0;
  virtual void exitQuantumCircuitDeclaration(qiskitParser::QuantumCircuitDeclarationContext *ctx) = 0;

  virtual void enterCircuitName(qiskitParser::CircuitNameContext *ctx) = 0;
  virtual void exitCircuitName(qiskitParser::CircuitNameContext *ctx) = 0;

  virtual void enterQuantumStatement(qiskitParser::QuantumStatementContext *ctx) = 0;
  virtual void exitQuantumStatement(qiskitParser::QuantumStatementContext *ctx) = 0;

  virtual void enterMeasurementStatement(qiskitParser::MeasurementStatementContext *ctx) = 0;
  virtual void exitMeasurementStatement(qiskitParser::MeasurementStatementContext *ctx) = 0;

  virtual void enterQuantumGateCall(qiskitParser::QuantumGateCallContext *ctx) = 0;
  virtual void exitQuantumGateCall(qiskitParser::QuantumGateCallContext *ctx) = 0;

  virtual void enterQuantumGateName(qiskitParser::QuantumGateNameContext *ctx) = 0;
  virtual void exitQuantumGateName(qiskitParser::QuantumGateNameContext *ctx) = 0;

  virtual void enterQuantumMeasurement(qiskitParser::QuantumMeasurementContext *ctx) = 0;
  virtual void exitQuantumMeasurement(qiskitParser::QuantumMeasurementContext *ctx) = 0;

  virtual void enterQuantumBarrier(qiskitParser::QuantumBarrierContext *ctx) = 0;
  virtual void exitQuantumBarrier(qiskitParser::QuantumBarrierContext *ctx) = 0;

  virtual void enterGateArgumentList(qiskitParser::GateArgumentListContext *ctx) = 0;
  virtual void exitGateArgumentList(qiskitParser::GateArgumentListContext *ctx) = 0;

  virtual void enterMeasurementArgumentList(qiskitParser::MeasurementArgumentListContext *ctx) = 0;
  virtual void exitMeasurementArgumentList(qiskitParser::MeasurementArgumentListContext *ctx) = 0;

  virtual void enterExpression(qiskitParser::ExpressionContext *ctx) = 0;
  virtual void exitExpression(qiskitParser::ExpressionContext *ctx) = 0;

  virtual void enterExpressionTerminator(qiskitParser::ExpressionTerminatorContext *ctx) = 0;
  virtual void exitExpressionTerminator(qiskitParser::ExpressionTerminatorContext *ctx) = 0;

  virtual void enterListExpression(qiskitParser::ListExpressionContext *ctx) = 0;
  virtual void exitListExpression(qiskitParser::ListExpressionContext *ctx) = 0;

  virtual void enterIntegerMeasurementArgument(qiskitParser::IntegerMeasurementArgumentContext *ctx) = 0;
  virtual void exitIntegerMeasurementArgument(qiskitParser::IntegerMeasurementArgumentContext *ctx) = 0;

  virtual void enterQubitMeasurementArgument(qiskitParser::QubitMeasurementArgumentContext *ctx) = 0;
  virtual void exitQubitMeasurementArgument(qiskitParser::QubitMeasurementArgumentContext *ctx) = 0;

  virtual void enterQuantumOperation(qiskitParser::QuantumOperationContext *ctx) = 0;
  virtual void exitQuantumOperation(qiskitParser::QuantumOperationContext *ctx) = 0;

  virtual void enterExpression_1(qiskitParser::Expression_1Context *ctx) = 0;
  virtual void exitExpression_1(qiskitParser::Expression_1Context *ctx) = 0;

  virtual void enterTerm(qiskitParser::TermContext *ctx) = 0;
  virtual void exitTerm(qiskitParser::TermContext *ctx) = 0;

  virtual void enterFactor(qiskitParser::FactorContext *ctx) = 0;
  virtual void exitFactor(qiskitParser::FactorContext *ctx) = 0;

  virtual void enterQuantumCount(qiskitParser::QuantumCountContext *ctx) = 0;
  virtual void exitQuantumCount(qiskitParser::QuantumCountContext *ctx) = 0;

  virtual void enterClassicalCount(qiskitParser::ClassicalCountContext *ctx) = 0;
  virtual void exitClassicalCount(qiskitParser::ClassicalCountContext *ctx) = 0;

  virtual void enterQubitReference(qiskitParser::QubitReferenceContext *ctx) = 0;
  virtual void exitQubitReference(qiskitParser::QubitReferenceContext *ctx) = 0;

  virtual void enterClassicalReference(qiskitParser::ClassicalReferenceContext *ctx) = 0;
  virtual void exitClassicalReference(qiskitParser::ClassicalReferenceContext *ctx) = 0;


};

}  // namespace qiskit
