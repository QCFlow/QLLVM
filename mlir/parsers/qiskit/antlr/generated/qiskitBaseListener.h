
// Generated from qiskit.g4 by ANTLR 4.9.2

#pragma once


#include "antlr4-runtime.h"
#include "qiskitListener.h"


namespace qiskit {

/**
 * This class provides an empty implementation of qiskitListener,
 * which can be extended to create a listener which only needs to handle a subset
 * of the available methods.
 */
class  qiskitBaseListener : public qiskitListener {
public:

  virtual void enterProgram(qiskitParser::ProgramContext * /*ctx*/) override { }
  virtual void exitProgram(qiskitParser::ProgramContext * /*ctx*/) override { }

  virtual void enterQuantumCircuitDeclaration(qiskitParser::QuantumCircuitDeclarationContext * /*ctx*/) override { }
  virtual void exitQuantumCircuitDeclaration(qiskitParser::QuantumCircuitDeclarationContext * /*ctx*/) override { }

  virtual void enterCircuitName(qiskitParser::CircuitNameContext * /*ctx*/) override { }
  virtual void exitCircuitName(qiskitParser::CircuitNameContext * /*ctx*/) override { }

  virtual void enterQuantumStatement(qiskitParser::QuantumStatementContext * /*ctx*/) override { }
  virtual void exitQuantumStatement(qiskitParser::QuantumStatementContext * /*ctx*/) override { }

  virtual void enterMeasurementStatement(qiskitParser::MeasurementStatementContext * /*ctx*/) override { }
  virtual void exitMeasurementStatement(qiskitParser::MeasurementStatementContext * /*ctx*/) override { }

  virtual void enterQuantumGateCall(qiskitParser::QuantumGateCallContext * /*ctx*/) override { }
  virtual void exitQuantumGateCall(qiskitParser::QuantumGateCallContext * /*ctx*/) override { }

  virtual void enterQuantumGateName(qiskitParser::QuantumGateNameContext * /*ctx*/) override { }
  virtual void exitQuantumGateName(qiskitParser::QuantumGateNameContext * /*ctx*/) override { }

  virtual void enterQuantumMeasurement(qiskitParser::QuantumMeasurementContext * /*ctx*/) override { }
  virtual void exitQuantumMeasurement(qiskitParser::QuantumMeasurementContext * /*ctx*/) override { }

  virtual void enterQuantumBarrier(qiskitParser::QuantumBarrierContext * /*ctx*/) override { }
  virtual void exitQuantumBarrier(qiskitParser::QuantumBarrierContext * /*ctx*/) override { }

  virtual void enterGateArgumentList(qiskitParser::GateArgumentListContext * /*ctx*/) override { }
  virtual void exitGateArgumentList(qiskitParser::GateArgumentListContext * /*ctx*/) override { }

  virtual void enterMeasurementArgumentList(qiskitParser::MeasurementArgumentListContext * /*ctx*/) override { }
  virtual void exitMeasurementArgumentList(qiskitParser::MeasurementArgumentListContext * /*ctx*/) override { }

  virtual void enterExpression(qiskitParser::ExpressionContext * /*ctx*/) override { }
  virtual void exitExpression(qiskitParser::ExpressionContext * /*ctx*/) override { }

  virtual void enterExpressionTerminator(qiskitParser::ExpressionTerminatorContext * /*ctx*/) override { }
  virtual void exitExpressionTerminator(qiskitParser::ExpressionTerminatorContext * /*ctx*/) override { }

  virtual void enterListExpression(qiskitParser::ListExpressionContext * /*ctx*/) override { }
  virtual void exitListExpression(qiskitParser::ListExpressionContext * /*ctx*/) override { }

  virtual void enterIntegerMeasurementArgument(qiskitParser::IntegerMeasurementArgumentContext * /*ctx*/) override { }
  virtual void exitIntegerMeasurementArgument(qiskitParser::IntegerMeasurementArgumentContext * /*ctx*/) override { }

  virtual void enterQubitMeasurementArgument(qiskitParser::QubitMeasurementArgumentContext * /*ctx*/) override { }
  virtual void exitQubitMeasurementArgument(qiskitParser::QubitMeasurementArgumentContext * /*ctx*/) override { }

  virtual void enterQuantumOperation(qiskitParser::QuantumOperationContext * /*ctx*/) override { }
  virtual void exitQuantumOperation(qiskitParser::QuantumOperationContext * /*ctx*/) override { }

  virtual void enterExpression_1(qiskitParser::Expression_1Context * /*ctx*/) override { }
  virtual void exitExpression_1(qiskitParser::Expression_1Context * /*ctx*/) override { }

  virtual void enterTerm(qiskitParser::TermContext * /*ctx*/) override { }
  virtual void exitTerm(qiskitParser::TermContext * /*ctx*/) override { }

  virtual void enterFactor(qiskitParser::FactorContext * /*ctx*/) override { }
  virtual void exitFactor(qiskitParser::FactorContext * /*ctx*/) override { }

  virtual void enterQuantumCount(qiskitParser::QuantumCountContext * /*ctx*/) override { }
  virtual void exitQuantumCount(qiskitParser::QuantumCountContext * /*ctx*/) override { }

  virtual void enterClassicalCount(qiskitParser::ClassicalCountContext * /*ctx*/) override { }
  virtual void exitClassicalCount(qiskitParser::ClassicalCountContext * /*ctx*/) override { }

  virtual void enterQubitReference(qiskitParser::QubitReferenceContext * /*ctx*/) override { }
  virtual void exitQubitReference(qiskitParser::QubitReferenceContext * /*ctx*/) override { }

  virtual void enterClassicalReference(qiskitParser::ClassicalReferenceContext * /*ctx*/) override { }
  virtual void exitClassicalReference(qiskitParser::ClassicalReferenceContext * /*ctx*/) override { }


  virtual void enterEveryRule(antlr4::ParserRuleContext * /*ctx*/) override { }
  virtual void exitEveryRule(antlr4::ParserRuleContext * /*ctx*/) override { }
  virtual void visitTerminal(antlr4::tree::TerminalNode * /*node*/) override { }
  virtual void visitErrorNode(antlr4::tree::ErrorNode * /*node*/) override { }

};

}  // namespace qiskit
