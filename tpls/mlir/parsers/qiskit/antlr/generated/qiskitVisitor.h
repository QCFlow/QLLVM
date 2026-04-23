
// Generated from qiskit.g4 by ANTLR 4.9.2

#pragma once


#include "antlr4-runtime.h"
#include "qiskitParser.h"


namespace qiskit {

/**
 * This class defines an abstract visitor for a parse tree
 * produced by qiskitParser.
 */
class  qiskitVisitor : public antlr4::tree::AbstractParseTreeVisitor {
public:

  /**
   * Visit parse trees produced by qiskitParser.
   */
    virtual antlrcpp::Any visitProgram(qiskitParser::ProgramContext *context) = 0;

    virtual antlrcpp::Any visitQuantumCircuitDeclaration(qiskitParser::QuantumCircuitDeclarationContext *context) = 0;

    virtual antlrcpp::Any visitCircuitName(qiskitParser::CircuitNameContext *context) = 0;

    virtual antlrcpp::Any visitQuantumStatement(qiskitParser::QuantumStatementContext *context) = 0;

    virtual antlrcpp::Any visitMeasurementStatement(qiskitParser::MeasurementStatementContext *context) = 0;

    virtual antlrcpp::Any visitQuantumGateCall(qiskitParser::QuantumGateCallContext *context) = 0;

    virtual antlrcpp::Any visitQuantumGateName(qiskitParser::QuantumGateNameContext *context) = 0;

    virtual antlrcpp::Any visitQuantumMeasurement(qiskitParser::QuantumMeasurementContext *context) = 0;

    virtual antlrcpp::Any visitQuantumBarrier(qiskitParser::QuantumBarrierContext *context) = 0;

    virtual antlrcpp::Any visitGateArgumentList(qiskitParser::GateArgumentListContext *context) = 0;

    virtual antlrcpp::Any visitMeasurementArgumentList(qiskitParser::MeasurementArgumentListContext *context) = 0;

    virtual antlrcpp::Any visitExpression(qiskitParser::ExpressionContext *context) = 0;

    virtual antlrcpp::Any visitExpressionTerminator(qiskitParser::ExpressionTerminatorContext *context) = 0;

    virtual antlrcpp::Any visitListExpression(qiskitParser::ListExpressionContext *context) = 0;

    virtual antlrcpp::Any visitIntegerMeasurementArgument(qiskitParser::IntegerMeasurementArgumentContext *context) = 0;

    virtual antlrcpp::Any visitQubitMeasurementArgument(qiskitParser::QubitMeasurementArgumentContext *context) = 0;

    virtual antlrcpp::Any visitQuantumOperation(qiskitParser::QuantumOperationContext *context) = 0;

    virtual antlrcpp::Any visitExpression_1(qiskitParser::Expression_1Context *context) = 0;

    virtual antlrcpp::Any visitTerm(qiskitParser::TermContext *context) = 0;

    virtual antlrcpp::Any visitFactor(qiskitParser::FactorContext *context) = 0;

    virtual antlrcpp::Any visitQuantumCount(qiskitParser::QuantumCountContext *context) = 0;

    virtual antlrcpp::Any visitClassicalCount(qiskitParser::ClassicalCountContext *context) = 0;

    virtual antlrcpp::Any visitQubitReference(qiskitParser::QubitReferenceContext *context) = 0;

    virtual antlrcpp::Any visitClassicalReference(qiskitParser::ClassicalReferenceContext *context) = 0;


};

}  // namespace qiskit
