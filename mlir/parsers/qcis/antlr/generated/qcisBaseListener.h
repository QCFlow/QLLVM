
// Generated from qcis.g4 by ANTLR 4.9.2

#pragma once


#include "antlr4-runtime.h"
#include "qcisListener.h"


namespace qcis {

/**
 * This class provides an empty implementation of qcisListener,
 * which can be extended to create a listener which only needs to handle a subset
 * of the available methods.
 */
class  qcisBaseListener : public qcisListener {
public:

  virtual void enterProgram(qcisParser::ProgramContext * /*ctx*/) override { }
  virtual void exitProgram(qcisParser::ProgramContext * /*ctx*/) override { }

  virtual void enterQuantumCircuitDeclaration(qcisParser::QuantumCircuitDeclarationContext * /*ctx*/) override { }
  virtual void exitQuantumCircuitDeclaration(qcisParser::QuantumCircuitDeclarationContext * /*ctx*/) override { }

  virtual void enterCircuitName(qcisParser::CircuitNameContext * /*ctx*/) override { }
  virtual void exitCircuitName(qcisParser::CircuitNameContext * /*ctx*/) override { }

  virtual void enterQuantumStatement(qcisParser::QuantumStatementContext * /*ctx*/) override { }
  virtual void exitQuantumStatement(qcisParser::QuantumStatementContext * /*ctx*/) override { }

  virtual void enterMeasurementStatement(qcisParser::MeasurementStatementContext * /*ctx*/) override { }
  virtual void exitMeasurementStatement(qcisParser::MeasurementStatementContext * /*ctx*/) override { }

  virtual void enterQuantumGateCall(qcisParser::QuantumGateCallContext * /*ctx*/) override { }
  virtual void exitQuantumGateCall(qcisParser::QuantumGateCallContext * /*ctx*/) override { }

  virtual void enterQuantumBarrier(qcisParser::QuantumBarrierContext * /*ctx*/) override { }
  virtual void exitQuantumBarrier(qcisParser::QuantumBarrierContext * /*ctx*/) override { }

  virtual void enterQuantumGateName(qcisParser::QuantumGateNameContext * /*ctx*/) override { }
  virtual void exitQuantumGateName(qcisParser::QuantumGateNameContext * /*ctx*/) override { }

  virtual void enterQuantumMeasurement(qcisParser::QuantumMeasurementContext * /*ctx*/) override { }
  virtual void exitQuantumMeasurement(qcisParser::QuantumMeasurementContext * /*ctx*/) override { }

  virtual void enterGateArgumentList(qcisParser::GateArgumentListContext * /*ctx*/) override { }
  virtual void exitGateArgumentList(qcisParser::GateArgumentListContext * /*ctx*/) override { }

  virtual void enterMeasurementArgumentList(qcisParser::MeasurementArgumentListContext * /*ctx*/) override { }
  virtual void exitMeasurementArgumentList(qcisParser::MeasurementArgumentListContext * /*ctx*/) override { }

  virtual void enterExpressionArgument(qcisParser::ExpressionArgumentContext * /*ctx*/) override { }
  virtual void exitExpressionArgument(qcisParser::ExpressionArgumentContext * /*ctx*/) override { }

  virtual void enterIntegerGateArgument(qcisParser::IntegerGateArgumentContext * /*ctx*/) override { }
  virtual void exitIntegerGateArgument(qcisParser::IntegerGateArgumentContext * /*ctx*/) override { }

  virtual void enterExpression(qcisParser::ExpressionContext * /*ctx*/) override { }
  virtual void exitExpression(qcisParser::ExpressionContext * /*ctx*/) override { }

  virtual void enterExpressionTerminator(qcisParser::ExpressionTerminatorContext * /*ctx*/) override { }
  virtual void exitExpressionTerminator(qcisParser::ExpressionTerminatorContext * /*ctx*/) override { }

  virtual void enterListExpression(qcisParser::ListExpressionContext * /*ctx*/) override { }
  virtual void exitListExpression(qcisParser::ListExpressionContext * /*ctx*/) override { }

  virtual void enterIntegerMeasurementArgument(qcisParser::IntegerMeasurementArgumentContext * /*ctx*/) override { }
  virtual void exitIntegerMeasurementArgument(qcisParser::IntegerMeasurementArgumentContext * /*ctx*/) override { }

  virtual void enterQubitMeasurementArgument(qcisParser::QubitMeasurementArgumentContext * /*ctx*/) override { }
  virtual void exitQubitMeasurementArgument(qcisParser::QubitMeasurementArgumentContext * /*ctx*/) override { }

  virtual void enterQuantumCount(qcisParser::QuantumCountContext * /*ctx*/) override { }
  virtual void exitQuantumCount(qcisParser::QuantumCountContext * /*ctx*/) override { }

  virtual void enterClassicalCount(qcisParser::ClassicalCountContext * /*ctx*/) override { }
  virtual void exitClassicalCount(qcisParser::ClassicalCountContext * /*ctx*/) override { }

  virtual void enterQubitReference(qcisParser::QubitReferenceContext * /*ctx*/) override { }
  virtual void exitQubitReference(qcisParser::QubitReferenceContext * /*ctx*/) override { }

  virtual void enterGateParameter(qcisParser::GateParameterContext * /*ctx*/) override { }
  virtual void exitGateParameter(qcisParser::GateParameterContext * /*ctx*/) override { }

  virtual void enterClassicalReference(qcisParser::ClassicalReferenceContext * /*ctx*/) override { }
  virtual void exitClassicalReference(qcisParser::ClassicalReferenceContext * /*ctx*/) override { }


  virtual void enterEveryRule(antlr4::ParserRuleContext * /*ctx*/) override { }
  virtual void exitEveryRule(antlr4::ParserRuleContext * /*ctx*/) override { }
  virtual void visitTerminal(antlr4::tree::TerminalNode * /*node*/) override { }
  virtual void visitErrorNode(antlr4::tree::ErrorNode * /*node*/) override { }

};

}  // namespace qcis
